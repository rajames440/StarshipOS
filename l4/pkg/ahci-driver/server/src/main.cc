/*
 * Copyright (C) 2015, 2017-2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <map>
#include <vector>

#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/re/util/shared_cap>
#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>

#include <terminate_handler-l4>

#include "ahci_partition.h"
#include "ahci_port.h"
#include "ahci_device.h"
#include "hba.h"

#include "debug.h" // needs to come before liblock-dev includes
#include <l4/libblock-device/block_device_mgr.h>
#include <l4/libblock-device/virtio_client.h>

static char const *const usage_str =
"Usage: %s [-vqA] [--client CAP --device UUID [--ds-max NUM] [--readonly]]\n\n"
"Options:\n"
" -v   Verbose mode.\n"
" -q   Quiet mode (do not print any warnings).\n"
" -A   Disable check for address width of device.\n"
"      Only do this if all physical memory is guaranteed to be below 4GB\n"
" --client CAP    Add a static client via the CAP capability\n"
" --device UUID   Specify the UUID of the device or partition\n"
" --ds-max NUM    Specify maximum number of dataspaces the client can register\n"
" --readonly      Only allow readonly access to the device\n";

struct Ahci_device_factory
{
  using Device_type = Ahci::Device;
  using Client_type = Block_device::Virtio_client<Device_type>;

  static cxx::unique_ptr<Client_type>
  create_client(cxx::Ref_ptr<Device_type> const &dev, unsigned numds, bool readonly)
  {
    return cxx::make_unique<Client_type>(dev, numds, readonly);
  }

  static cxx::Ref_ptr<Device_type>
  create_partition(cxx::Ref_ptr<Device_type> const &dev, unsigned partition_id,
                   Block_device::Partition_info const &pi)
  {
    return cxx::Ref_ptr<Device_type>(new Ahci::Partitioned_device(dev, partition_id, pi));
  }
};


using Base_device_mgr = Block_device::Device_mgr<Block_device::Device,
                                                 Ahci_device_factory>;

class Blk_mgr
: public Base_device_mgr,
  public L4::Epiface_t<Blk_mgr, L4::Factory>
{
  class Deletion_irq : public L4::Irqep_t<Deletion_irq>
  {
  public:
    void handle_irq()
    { _parent->check_clients(); }

    Deletion_irq(Blk_mgr *parent) : _parent{parent} {}

  private:
    Blk_mgr *_parent;
  };

public:
  Blk_mgr(L4Re::Util::Object_registry *registry)
  : Base_device_mgr(registry),
    _del_irq(this)
  {
    auto c = L4Re::chkcap(registry->register_irq_obj(&_del_irq),
                          "Creating IRQ for IPC gate deletion notifications.");
    L4Re::chksys(L4Re::Env::env()->main_thread()->register_del_irq(c),
                 "Registering deletion IRQ at the thread.");
  }

  long op_create(L4::Factory::Rights, L4::Ipc::Cap<void> &res, l4_umword_t,
                 L4::Ipc::Varg_list_ref valist)
  {
    Dbg::trace().printf("Client requests connection.\n");

    // default values
    std::string device;
    int num_ds = 2;
    bool readonly = false;
    int max_slots = 0;

    for (L4::Ipc::Varg p: valist)
      {
        if (!p.is_of<char const *>())
          {
            Dbg::warn().printf("String parameter expected.\n");
            return -L4_EINVAL;
          }

        std::string device_param;
        if (parse_string_param(p, "device=", &device_param))
          {
            long ret = parse_device_name(device_param, device);
            if (ret < 0)
              return ret;
            continue;
          }
        if (parse_int_param(p, "ds-max=", &num_ds))
          {
            if (num_ds <= 0 || num_ds > 256) // sanity check with arbitrary limit
              {
                Dbg::warn().printf("Invalid range for parameter 'ds-max'. "
                                   "Number must be between 1 and 256.\n");
                return -L4_EINVAL;
              }
            continue;
          }
        if (parse_int_param(p, "slot-max=", &max_slots))
          continue;
        if (strncmp(p.value<char const *>(), "read-only", p.length()) == 0)
          readonly = true;
      }

    if (device.empty())
      {
        Dbg::warn().printf("Parameter 'device=' not found. "
                           "Device UUID is required.\n");
        return -L4_EINVAL;
      }

    L4::Cap<void> cap;
    int ret = create_dynamic_client(device, -1, num_ds, &cap, readonly,
                [max_slots](Block_device::Device *d)
                  {
                    auto *part = dynamic_cast<Ahci::Partitioned_device *>(d);
                    if (part)
                      part->set_max_in_flight(max_slots);
                    else
                      if (max_slots)
                        Dbg::warn("slot-max parameter ignored for full disk access.\n");
                  });
    if (ret >= 0)
      {
        res = L4::Ipc::make_cap(cap, L4_CAP_FPAGE_RWSD);
        L4::cap_cast<L4::Kobject>(cap)->dec_refcnt(1);
      }

    return (ret == -L4_ENODEV && _scan_in_progress) ? -L4_EAGAIN : ret;
  }

  void scan_finished()
  { _scan_in_progress = false; }

private:
  static bool parse_string_param(L4::Ipc::Varg const &param, char const *prefix,
                                 std::string *out)
  {
    l4_size_t headlen = strlen(prefix);

    if (param.length() < headlen)
      return false;

    char const *pstr = param.value<char const *>();

    if (strncmp(pstr, prefix, headlen) != 0)
      return false;

    *out = std::string(pstr + headlen, strnlen(pstr, param.length()) - headlen);

    return true;
  }

  static bool parse_int_param(L4::Ipc::Varg const &param, char const *prefix,
                              int *out)
  {
    l4_size_t headlen = strlen(prefix);

    if (param.length() < headlen)
      return false;

    char const *pstr = param.value<char const *>();

    if (strncmp(pstr, prefix, headlen) != 0)
      return false;

    std::string tail(pstr + headlen, param.length() - headlen);

    char *endp;
    long num = strtol(tail.c_str(), &endp, 10);

    if (num < INT_MIN || num > INT_MAX || *endp != '\0')
      {
        Dbg::warn().printf("Bad parameter '%s'. Number required.\n", prefix);
        L4Re::chksys(-L4_EINVAL);
      }

    *out = num;

    return true;
  }

  Deletion_irq _del_irq;
  bool _scan_in_progress = true;
};

struct Client_opts
{
  bool add_client(Blk_mgr *blk_mgr)
  {
    if (capname)
      {
        if (device.empty())
          {
            Err().printf("No device for client '%s' given. "
                         "Please specify a device.\n", capname);
            return false;
          }

        auto cap = L4Re::Env::env()->get_cap<L4::Rcv_endpoint>(capname);
        if (!cap.is_valid())
          {
            Err().printf("Client capability '%s' no found.\n", capname);
            return false;
          }

        int mx = slot_max;
        blk_mgr->add_static_client(cap, device.c_str(), -1, ds_max, readonly,
          [mx](Block_device::Device *d)
            {
              auto *part = dynamic_cast<Ahci::Partitioned_device *>(d);
              if (part)
                part->set_max_in_flight(mx);
              else
                if (mx)
                  Dbg::warn("slot-max parameter ignored for full disk access.\n");
            });
      }

    return true;
  }

  const char *capname = nullptr;
  std::string device;
  int ds_max = 2;
  bool readonly = false;
  int slot_max = 0;
};

static Block_device::Errand::Errand_server server;
static Blk_mgr drv(server.registry());
std::vector<cxx::unique_ptr<Ahci::Hba>> _hbas;
unsigned static devices_in_scan = 0;

static int
parse_args(int argc, char *const *argv)
{
  int debug_level = 1;

  enum
  {
    OPT_CLIENT,
    OPT_DEVICE,
    OPT_DS_MAX,
    OPT_SLOT_MAX,
    OPT_READONLY,
  };

  struct option const loptions[] =
  {
    { "verbose",       no_argument,       NULL, 'v' },
    { "quiet",         no_argument,       NULL, 'q' },
    { "check_address", no_argument,       NULL, 'A' },
    { "client",        required_argument, NULL,  OPT_CLIENT },
    { "device",        required_argument, NULL,  OPT_DEVICE },
    { "ds-max",        required_argument, NULL,  OPT_DS_MAX },
    { "slot-max",      required_argument, NULL,  OPT_SLOT_MAX },
    { "readonly",      no_argument,       NULL,  OPT_READONLY },
    { 0, 0, 0, 0 },
  };

  Client_opts opts;
  for (;;)
    {
      int opt = getopt_long(argc, argv, "vqA", loptions, NULL);
      if (opt == -1)
        break;

      switch (opt)
        {
        case 'v':
          debug_level <<= 1;
          ++debug_level;
          break;
        case 'q':
          debug_level = 0;
          break;
        case 'A':
          Ahci::Hba::check_address_width = false;
          break;
        case OPT_CLIENT:
          if (!opts.add_client(&drv))
            return 1;
          opts = Client_opts();
          opts.capname = optarg;
          break;
        case OPT_DEVICE:
          if (Blk_mgr::parse_device_name(optarg, opts.device) < 0)
            {
              Dbg::warn().printf("Invalid device name parameter.\n");
              return -1;
            }
          break;
        case OPT_DS_MAX:
          opts.ds_max = atoi(optarg);
          break;
        case OPT_SLOT_MAX:
          opts.slot_max = atoi(optarg);
          break;
        case OPT_READONLY:
          opts.readonly = true;
          break;
        default:
          Dbg::warn().printf(usage_str, argv[0]);
          return -1;
        }
    }

  if (!opts.add_client(&drv))
    return 1;

  Dbg::set_level(debug_level);
  return optind;
}

static void
device_scan_finished()
{
  if (--devices_in_scan > 0)
    return;

  drv.scan_finished();
  if (!server.registry()->register_obj(&drv, "svr").is_valid())
    Dbg::warn().printf("Capability 'svr' not found. No dynamic clients accepted.\n");
  else
    Dbg::trace().printf("Device now accepts new clients.\n");
}

static L4Re::Util::Shared_cap<L4Re::Dma_space>
create_dma_space(L4::Cap<L4vbus::Vbus> bus, long unsigned id)
{
  static std::map<long unsigned, L4Re::Util::Shared_cap<L4Re::Dma_space>> spaces;

  auto ires = spaces.find(id);
  if (ires != spaces.end())
    return ires->second;

  auto dma = L4Re::chkcap(L4Re::Util::make_shared_cap<L4Re::Dma_space>(),
                          "Allocate capability for DMA space.");
  L4Re::chksys(L4Re::Env::env()->user_factory()->create(dma.get()),
               "Create DMA space.");
  L4Re::chksys(
    bus->assign_dma_domain(id, L4VBUS_DMAD_BIND | L4VBUS_DMAD_L4RE_DMA_SPACE,
                           dma.get()),
    "Assignment of DMA domain.");

  spaces[id] = dma;
  return dma;
}

static void
device_discovery(L4::Cap<L4vbus::Vbus> bus, L4::Cap<L4::Icu> icu)
{
  Dbg::info().printf("Starting device discovery.\n");

  L4vbus::Pci_dev child;

  l4vbus_device_t di;
  auto root = bus->root();

  // make sure that we don't finish device scan before the while loop is done
  ++devices_in_scan;

  while (root.next_device(&child, L4VBUS_MAX_DEPTH, &di) == L4_EOK)
    {
      Dbg::trace().printf("Scanning child 0x%lx.\n", child.dev_handle());

      if (Ahci::Hba::is_ahci_hba(child, di))
        {
          unsigned long id = -1UL;
          for (auto i = 0u; i < di.num_resources; ++i)
            {
              l4vbus_resource_t res;
              L4Re::chksys(child.get_resource(i, &res), "Getting resource.");
              if (res.type == L4VBUS_RESOURCE_DMA_DOMAIN)
                {
                  id = res.start;
                  Dbg::trace().printf("Using device's DMA domain %lu.\n",
                                      res.start);
                  break;
                }
            }

          if (id == -1UL)
            Dbg::trace().printf("Using VBUS global DMA domain.\n");

          try
            {
              auto hba = cxx::make_unique<Ahci::Hba>(child,
                                                     create_dma_space(bus, id));
              hba->register_interrupt_handler(icu, server.registry());
              _hbas.push_back(cxx::move(hba));
            }
          catch (L4::Runtime_error const &e)
            {
              Err().printf("%s: %s\n", e.str(), e.extra_str());
              continue;
            }

          devices_in_scan += _hbas.back()->num_ports();

          _hbas.back()->scan_ports(
            [=](Ahci::Ahci_port *port)
              {
                if (port && Ahci::Ahci_device::is_compatible_device(port))
                  drv.add_disk(cxx::make_ref_obj<Ahci::Ahci_device>(port),
                               device_scan_finished);
                else
                  device_scan_finished();
              });
        }
    }

  // marks the end of the device detection loop
  device_scan_finished();

  Dbg::info().printf("All devices scanned.\n");
}

static void
setup_hardware()
{
  auto vbus = L4Re::chkcap(L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus"),
                           "Get 'vbus' capability.", -L4_ENOENT);

  L4vbus::Icu icudev;
  L4Re::chksys(vbus->root().device_by_hid(&icudev, "L40009"),
               "Look for ICU device.");
  auto icu = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Icu>(),
                          "Allocate ICU capability.");
  L4Re::chksys(icudev.vicu(icu), "Request ICU capability.");

  device_discovery(vbus, icu);
}

int
main(int argc, char *const *argv)
{
  Dbg::set_level(3);

  int arg_idx = parse_args(argc, argv);
  if (arg_idx < 0)
    return arg_idx;

  Dbg::info().printf("AHCI driver says hello.\n");

  Block_device::Errand::set_server_iface(&server);
  setup_hardware();

  Dbg::trace().printf("Beginning server loop...\n");
  server.loop();

  return 0;
}
