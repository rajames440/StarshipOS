/*
 * Copyright (C) 2016-2020, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Manuel von Oltersdorff-Kalettka <manuel.kalettka@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#include <l4/re/util/meta>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>

#include <l4/sys/factory>
#include <l4/sys/task>

#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_varg>
#include <l4/cxx/string>

#include <vector>
#include <string>
#include <terminate_handler-l4>

#include "debug.h"
#include "options.h"
#include "switch.h"
#include "vlan.h"

/**
 * \defgroup virtio_net_switch Virtio Net Switch
 *
 * A virtual network switch that can be used as defined in the virtio protocol.
 *
 * The abstraction of a single connection with a network device (also called
 * client) from the switch's perspective is a port. A client can register
 * multiple ports on the switch. The communication between a client and the
 * switch happens via IRQs, MMIO and shared memory as defined by the Virtio
 * protocol. The switch supports VLANs and ports can be either 'access' or
 * 'trunk' ports.
 * The optionally available monitor port receives network traffic from all
 * ports, and the monitor can not send.
 *
 * \{
 */


/*
 * Registry for our server, used to register
 * - factory capability
 * - irq object for capability deletion irqs
 * - virtio host kick irqs
 */
static L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> server;

using Ds_vector = std::vector<L4::Cap<L4Re::Dataspace>>;
static std::shared_ptr<Ds_vector> trusted_dataspaces;

static bool
parse_int_param(L4::Ipc::Varg const &param, char const *prefix, int *out)
{
  l4_size_t headlen = strlen(prefix);

  if (param.length() < headlen)
    return false;

  char const *pstr = param.value<char const *>();

  if (strncmp(pstr, prefix, headlen) != 0)
    return false;

  std::string tail(pstr + headlen, param.length() - headlen);

  if (!parse_int_optstring(tail.c_str(), out))
    {
      Err(Err::Normal).printf("Bad parameter '%s'. Invalid number specified.\n",
                              prefix);
      throw L4::Runtime_error(-L4_EINVAL);
    }

  return true;
}

/**
 * The IPC interface for creating ports.
 *
 * The Switch factory provides an IPC interface to create ports. Ports are
 * the only option for a client to communicate with the switch and, thus, with
 * other network devices.
 *
 * The `Switch_factory` gets constructed when the net switch application gets
 * started. It thereafter gets registered on the switch's server to serve IPC
 * `create` calls.
 */
class Switch_factory : public L4::Epiface_t<Switch_factory, L4::Factory>
{
  /**
   * Implement the generic irq related part of the port
   */
  class Port : public L4virtio_port
  {
    // Irq used to notify the guest
    L4::Cap<L4::Irq> _device_notify_irq;

    L4::Cap<L4::Irq> device_notify_irq() const override
    { return _device_notify_irq; }

  public:
    Port(unsigned vq_max, unsigned num_ds, char const *name,
         l4_uint8_t const *mac)
    : L4virtio_port(vq_max, num_ds, name, mac) {}

    /** register the host IRQ and the port itself on the switch's server */
    void register_end_points(L4Re::Util::Object_registry* registry,
                             L4::Epiface *kick_irq)
    {
      // register virtio host kick irq
      _device_notify_irq = L4Re::chkcap(registry->register_irq_obj(kick_irq));

      // register virtio endpoint
      L4Re::chkcap(registry->register_obj(this));

      // decrement ref counter to get a notification when the last
      // external reference vanishes
      obj_cap()->dec_refcnt(1);
    }

    virtual ~Port()
    { server.registry()->unregister_obj(this); }
  };

  /**
   * Implement the irq related part of a switched port
   */
  class Switch_port : public Port
  {
    /**
     * IRQ endpoint on the port.
     *
     * Each port holds its own IRQ that gets triggered by the client whenever
     * there is a new outgoing request in the port's transmission queue or when
     * there is new space in the port's receive queue.
     *
     * A `Kick_irq` is constructed on port creation. At this time, it also gets
     * registered on the switch's server.
     */
    class Kick_irq : public L4::Irqep_t<Kick_irq>
    {
      Virtio_switch *_switch; /**< pointer to the net switch */
      L4virtio_port *_port;     /**< pointer to the associated port */

    public:
      /**
       * Callback for the IRQ
       *
       * This function redirects the call to `Virtio_switch::handle_l4virtio_port_tx`,
       * since the port cannot finish a transmission on its own.
       */
      void handle_irq()
      { _switch->handle_l4virtio_port_tx(_port); }

      Kick_irq(Virtio_switch *virtio_switch, L4virtio_port *port)
      : _switch{virtio_switch}, _port{port} {}
    };

    Kick_irq _kick_irq; /**< The IRQ to notify the client. */
    Kick_irq _reschedule_tx_irq;

  public:
    Switch_port(L4Re::Util::Object_registry *registry,
                Virtio_switch *virtio_switch, unsigned vq_max, unsigned num_ds,
                char const *name, l4_uint8_t const *mac)
    : Port(vq_max, num_ds, name, mac),
      _kick_irq(virtio_switch, this),
      _reschedule_tx_irq(virtio_switch, this)
    {
      register_end_points(registry, &_kick_irq);

      _pending_tx_reschedule =
        L4Re::chkcap(registry->register_irq_obj(&_reschedule_tx_irq),
                     "Register TX reschedule IRQ.");
      _pending_tx_reschedule->unmask();
    }

    virtual ~Switch_port()
    {
      // We need to delete the IRQ object created in register_irq_obj() ourselves
      L4::Cap<L4::Task>(L4Re::This_task)
        ->unmap(_kick_irq.obj_cap().fpage(),
                L4_FP_ALL_SPACES | L4_FP_DELETE_OBJ);
      server.registry()->unregister_obj(&_kick_irq);
    }
  };

  /**
   * Implement the irq related part of a monitor port
   */
  class Monitor_port : public Port
  {
    /**
     * Handle incoming irqs by
     * - handling pending outgoing requests
     * - dropping all incoming requests
     */
    class Kick_irq : public L4::Irqep_t<Kick_irq>
    {
      L4virtio_port *_port;

    public:
      /**
       * Callback for the IRQ
       *
       * A Monitor port processes only requests on its receive queue and drops
       * all requests on the transmit queue since it is not supposed to send
       * network request.
       */
      void handle_irq()
      {
        do
          {
            _port->tx_q()->disable_notify();
            _port->rx_q()->disable_notify();

            _port->drop_requests();

            _port->tx_q()->enable_notify();
            _port->rx_q()->enable_notify();

            L4virtio::wmb();
            L4virtio::rmb();
        }
        while (_port->tx_work_pending());
      }

      Kick_irq(L4virtio_port *port) : _port{port} {}
    };

    Kick_irq _kick_irq;

  public:
    Monitor_port(L4Re::Util::Object_registry* registry,
                 unsigned vq_max, unsigned num_ds, char const *name,
                 l4_uint8_t const *mac)
    : Port(vq_max, num_ds, name, mac), _kick_irq(this)
    { register_end_points(registry, &_kick_irq); }

    virtual ~Monitor_port()
    {
      // We need to delete the IRQ object created in register_irq_obj() ourselves
      L4::Cap<L4::Task>(L4Re::This_task)
        ->unmap(_kick_irq.obj_cap().fpage(),
                L4_FP_ALL_SPACES | L4_FP_DELETE_OBJ);
      server.registry()->unregister_obj(&_kick_irq);
    }
  };

  /*
   * Handle vanishing caps by telling the switch that a port might have gone
   */
  struct Del_cap_irq : public L4::Irqep_t<Del_cap_irq>
  {
  public:
    void handle_irq()
    { _switch->check_ports(); }

    Del_cap_irq(Virtio_switch *virtio_switch) : _switch{virtio_switch} {}

  private:
    Virtio_switch *_switch;
  };

  Virtio_switch *_virtio_switch; /**< pointer to the actual net switch object */

  /** maximum number of entries in a new virtqueueue created for a port */
  unsigned _vq_max_num;
  Del_cap_irq _del_cap_irq;

  /**
   * Evaluate an optional argument
   *
   * \param      opt          Optional argument.
   * \param[out] monitor      Set to true if argument is "type=monitor".
   * \param      name         Pointer to name.
   * \param      size         Size of name.
   * \param[out] vlan_access  Id of VLAN access port if "vlan=access=<id>" is
   *                          present.
   * \param[out] vlan_trunk   List of VLANs if "vlan=trunk=[<id>[,<id]*] is
   *                          present.
   * \param[out] vlan_trunk_all
   *                          Iff true, trunk port shall participate in all
   *                          VLANs. vlan_trunk will be ignored.
   */
  bool handle_opt_arg(L4::Ipc::Varg const &opt, bool &monitor,
                      char *name, size_t size,
                      l4_uint16_t &vlan_access,
                      std::vector<l4_uint16_t> &vlan_trunk,
                      bool *vlan_trunk_all,
                      l4_uint8_t mac[6], bool &mac_set)
  {
    assert(opt.is_of<char const *>());
    unsigned len = opt.length();
    const char *opt_str = opt.data();
    Err err(Err::Normal);


    if (len > 5)
      {
        if (!strncmp("type=", opt_str, 5))
          {
            if (!strncmp("type=monitor", opt_str, len))
              {
                monitor = true;
                return true;
              }
            else if (!strncmp("type=none", opt_str, len))
              return true;

            err.printf("Unknown type '%.*s'\n", opt.length() - 5, opt.data() + 5);
            return false;
          }
        else if (!strncmp("name=", opt_str, 5))
          {
            snprintf(name, size, "%.*s", opt.length() - 5, opt.data() + 5);
            return true;
          }
        else if (!strncmp("vlan=", opt_str, 5))
          {
            cxx::String str(opt_str + 5, strnlen(opt_str + 5, len - 5));
            cxx::String::Index idx;

            if ((idx = str.starts_with("access=")))
              {
                str = str.substr(idx);
                l4_uint16_t vid;
                int next = str.from_dec(&vid);
                if (next && next == str.len() && vlan_valid_id(vid))
                  vlan_access = vid;
                else
                  {
                    err.printf("Invalid VLAN access port id '%.*s'\n",
                               opt.length(), opt.data());
                    return false;
                  }
              }
            else if ((idx = str.starts_with("trunk=")))
              {
                int next;
                l4_uint16_t vid;
                str = str.substr(idx);
                if (str == cxx::String("all"))
                {
                  *vlan_trunk_all = true;
                  return true;
                }
                while ((next = str.from_dec(&vid)))
                  {
                    if (!vlan_valid_id(vid))
                      break;
                    vlan_trunk.push_back(vid);
                    if (next < str.len() && str[next] != ',')
                      break;
                    str = str.substr(next+1);
                  }

                if (vlan_trunk.empty() || !str.empty())
                  {
                    err.printf("Invalid VLAN trunk port spec '%.*s'\n",
                               opt.length(), opt.data());
                    return false;
                  }
              }
            else
              {
                err.printf("Invalid VLAN specification..\n");
                return false;
              }

            return true;
          }
        else if (!strncmp("mac=", opt_str, 4))
          {
            size_t const OPT_LEN = 4 /* mac= */ + 6*2 /* digits */ + 5 /* : */;
            // expect NUL terminated string for simplicity
            if (len > OPT_LEN && opt_str[OPT_LEN] == '\0' &&
                sscanf(opt_str+4, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0],
                       &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6)
              {
                mac_set = true;
                return true;
              }

            err.printf("Invalid mac address '%.*s'\n", len - 4, opt_str + 4);
            return false;
          }
      }

    err.printf("Unknown option '%.*s'\n", opt.length(), opt.data());
    return false;
  }

public:
  Switch_factory(Virtio_switch *virtio_switch, unsigned vq_max_num)
  : _virtio_switch{virtio_switch}, _vq_max_num{vq_max_num},
    _del_cap_irq{virtio_switch}
  {
    auto c = L4Re::chkcap(server.registry()->register_irq_obj(&_del_cap_irq));
    L4Re::chksys(L4Re::Env::env()->main_thread()->register_del_irq(c));
  };

  /**
   * Handle factory protocol
   *
   * This function is invoked after an incoming factory::create
   * request and creates a new port if possible.
   */
  long op_create(L4::Factory::Rights, L4::Ipc::Cap<void> &res,
                 l4_umword_t type, L4::Ipc::Varg_list_ref va)
  {
    Dbg warn(Dbg::Port, Dbg::Warn, "Port");
    Dbg info(Dbg::Port, Dbg::Info, "Port");

    info.printf("Incoming port request\n");

    // test for supported object types
    if (type != 0)
      {
        warn.printf("Invalid object type\n");
        return -L4_EINVAL;
      }

    bool monitor = false;
    char name[20] = "";
    unsigned arg_n = 2;
    l4_uint16_t vlan_access = 0;
    std::vector<l4_uint16_t> vlan_trunk;
    bool vlan_trunk_all = false;

    // Default MAC address. Might be overridden by a "mac=..." option.
    // First octet: 0x02
    // * bit 0: Individual/Group address bit
    // * bit 1: Universally/Locally Administered address bit
    // Last two octets are filled with port number.
    l4_uint8_t mac[6] = { 0x02, 0x08, 0x0f, 0x2a, 0x00, 0x00 };
    bool mac_set = false;
    int num_ds = 2;

    for (L4::Ipc::Varg opt: va)
      {
        if (!opt.is_of<char const *>())
          {
            warn.printf("Unexpected type for argument %d\n", arg_n);
            return -L4_EINVAL;
          }

        if (parse_int_param(opt, "ds-max=", &num_ds))
          {
            if (num_ds <= 0 || num_ds > 80)
              {
                Err(Err::Normal).printf("warning: client requested invalid number"
                                        " of data spaces: 0 < %d <= 80\n", num_ds);
                return -L4_EINVAL;
              }
          }
        else if (!handle_opt_arg(opt, monitor, name, sizeof(name), vlan_access,
                                 vlan_trunk, &vlan_trunk_all, mac, mac_set))
          return -L4_EINVAL;

        ++arg_n;
      }

    int port_num = _virtio_switch->port_available(monitor);
    if (port_num < 0)
      {
        warn.printf("No port available\n");
        return -L4_ENOMEM;
      }

    if (vlan_access && (!vlan_trunk.empty() || vlan_trunk_all))
      {
        warn.printf("Port cannot be access and trunk VLAN port simultaneously.\n");
        return -L4_EINVAL;
      }

    if (name[0])
      {
        // append port number
        unsigned len = strlen(name);
        snprintf(name + len, sizeof(name) - len, "[%d]", port_num);
      }
    else
      snprintf(name, sizeof(name), "%s[%d]", monitor ? "monitor" : "",
               port_num);

    info.printf("    Creating port %s%s\n", name,
                monitor ? " as monitor port" : "");

    if (!mac_set)
      {
        // assign a dedicated MAC address to the monitor interface
        // assuming we will never have more than 57000 (0xdea8) normal
        // ports
        if (monitor)
          {
            mac[4] = 0xde;
            mac[5] = 0xad;
          }
        else
          {
            mac[4] = (l4_uint8_t)(port_num >> 8);
            mac[5] = (l4_uint8_t)port_num;
          }
      }
    l4_uint8_t *mac_ptr = (mac_set || Options::get_options()->assign_mac())
                          ? mac : nullptr;

    // create port
    Port *port;
    if (monitor)
      {
        port = new Monitor_port(server.registry(), _vq_max_num, num_ds, name,
                                mac_ptr);
        port->set_monitor();

        if (vlan_access)
          warn.printf("vlan=access=<id> ignored on monitor ports!\n");
        if (!vlan_trunk.empty())
          warn.printf("vlan=trunk=... ignored on monitor ports!\n");
      }
    else
      {
        port = new Switch_port(server.registry(), _virtio_switch, _vq_max_num,
                               num_ds, name, mac_ptr);

        if (vlan_access)
          port->set_vlan_access(vlan_access);
        else if (vlan_trunk_all)
          port->set_vlan_trunk_all();
        else if (!vlan_trunk.empty())
          port->set_vlan_trunk(vlan_trunk);
      }

    port->add_trusted_dataspaces(trusted_dataspaces);
    if (!trusted_dataspaces->empty())
      port->enable_trusted_ds_validation();

    // hand port over to the switch
    bool added = monitor ? _virtio_switch->add_monitor_port(port)
                         : _virtio_switch->add_port(port);
    if (!added)
      {
        delete port;
        return -L4_ENOMEM;
      }
    res = L4::Ipc::make_cap(port->obj_cap(), L4_CAP_FPAGE_RWSD);

    info.printf("    Created port %s\n", name);
    return L4_EOK;
  };
};

#if CONFIG_VNS_IXL
/**
 * Implement the irq related part of an ixl port.
 */
class Ixl_hw_port : public Ixl_port
{
  template<typename Derived>
  class Port_irq : public L4::Irqep_t<Derived>
  {
  public:
    Port_irq(Virtio_switch *virtio_switch, Ixl_port *port)
    : _switch{virtio_switch}, _port{port} {}

  protected:
    Virtio_switch *_switch;
    Ixl_port *_port;
  };

  class Receive_irq : public Port_irq<Receive_irq>
  {
  public:
    using Port_irq::Port_irq;

    /**
     * Callback for the IRQ
     *
     * This function redirects the call to `Virtio_switch::handle_ixl_port_tx`,
     * since the port cannot finish a transmission on its own.
     */
    void handle_irq()
    {
      if (!_port->dev()->check_recv_irq(0))
        return;

      if (_switch->handle_ixl_port_tx(_port))
        _port->dev()->ack_recv_irq(0);
    }
  };

  class Reschedule_tx_irq : public Port_irq<Reschedule_tx_irq>
  {
  public:
    using Port_irq::Port_irq;

    void handle_irq()
    {
      if (_switch->handle_ixl_port_tx(_port))
        // Entire TX queue handled, re-enable the recv IRQ again.
        _port->dev()->ack_recv_irq(0);
    }
  };

  Receive_irq _recv_irq;
  Reschedule_tx_irq _reschedule_tx_irq;

public:
  Ixl_hw_port(L4Re::Util::Object_registry *registry,
              Virtio_switch *virtio_switch, Ixl::Ixl_device *dev)
  : Ixl_port(dev),
    _recv_irq(virtio_switch, this),
    _reschedule_tx_irq(virtio_switch, this)
  {
    L4::Cap<L4::Irq> recv_irq_cap = L4Re::chkcap(dev->get_recv_irq(0), "Get receive IRQ");
    L4Re::chkcap(registry->register_obj(&_recv_irq, recv_irq_cap),
                 "Register receive IRQ.");
    recv_irq_cap->unmask();

    _pending_tx_reschedule =
      L4Re::chkcap(registry->register_irq_obj(&_reschedule_tx_irq),
                   "Register TX reschedule IRQ.");
    _pending_tx_reschedule->unmask();
  }

  ~Ixl_hw_port() override
  {
    server.registry()->unregister_obj(&_recv_irq);
  }
};

static void
discover_ixl_devices(L4::Cap<L4vbus::Vbus> vbus, Virtio_switch *virtio_switch)
{
  struct Ixl::Dev_cfg cfg;
  // Configure the device in asynchronous notify mode.
  cfg.irq_timeout_ms = -1;

  // TODO: Support detecting multiple devices on a Vbus.
  // Setup the driver (also resets and initializes the NIC).
  Ixl::Ixl_device *dev = Ixl::Ixl_device::ixl_init(vbus, 0, cfg);
  if (!dev)
    // No Ixl supported device found, Ixl already printed an error message.
    return;

  Ixl_hw_port *hw_port = new Ixl_hw_port(server.registry(), virtio_switch, dev);
  if (!virtio_switch->add_port(hw_port))
    {
      Err().printf("error adding ixl port\n");
      delete hw_port;
    }
}
#endif

int main(int argc, char *argv[])
{
  trusted_dataspaces = std::make_shared<Ds_vector>();
  auto *opts = Options::parse_options(argc, argv, trusted_dataspaces);
  if (!opts)
    {
      Err().printf("Error during command line parsing.\n");
      return 1;
    }

  // Show welcome message if debug level is not set to quiet
  if (Dbg(Dbg::Core, Dbg::Warn).is_active())
    printf("Hello from l4virtio switch\n");

  Virtio_switch *virtio_switch = new Virtio_switch(opts->get_max_ports());

#if CONFIG_VNS_IXL
  auto vbus = L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus");
  if (vbus.is_valid())
    discover_ixl_devices(vbus, virtio_switch);
#endif

  Switch_factory *factory = new Switch_factory(virtio_switch,
                                               opts->get_virtq_max_num());
  L4::Cap<void> cap = server.registry()->register_obj(factory, "svr");
  if (!cap.is_valid())
    {
      Err().printf("error registering switch\n");
      return 2;
    }

  /*
   * server loop will handle 4 types of events
   * - Switch_factory
   *   - factory protocol
   *   - capability deletion
   *     - delegated to  Virtio_switch::check_ports()
   * - Switch_factory::Switch_port
   *   - irqs triggered by clients
   *     - delegated to Virtio_switch::handle_l4virtio_port_tx()
   * - Virtio_net_transfer
   *   - timeouts for pending transfer requests added by
   *     Port_iface::handle_request() via registered via
   *     L4::Epiface::server_iface()->add_timeout()
   */
  server.loop();
  return 0;
}

/**\}*/
