/*
 * Copyright (C) 2016-2018, 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#include "debug.h"
#include "switch.h"
#include "filter.h"

Virtio_switch::Virtio_switch(unsigned max_ports)
: _max_ports{max_ports},
  _max_used{0}
{
  _ports = new Port_iface *[max_ports]();
}

int
Virtio_switch::lookup_free_slot()
{
  for (unsigned idx = 0; idx < _max_ports; ++idx)
    if (!_ports[idx])
      return idx;

  return -1;
}

bool
Virtio_switch::add_port(Port_iface *port)
{
  if (!port->mac().is_unknown())
    for (unsigned idx = 0; idx < _max_ports; ++idx)
      if (_ports[idx] && _ports[idx]->mac() == port->mac())
        {
          Dbg(Dbg::Port, Dbg::Warn)
            .printf("Rejecting port '%s'. MAC address already in use.\n",
                    port->get_name());
          return false;
        }

  int idx = lookup_free_slot();
  if (idx < 0)
    return false;

  unsigned uidx = static_cast<unsigned>(idx);
  _ports[uidx] = port;
  if (_max_used == uidx)
    ++_max_used;

  return true;
}

bool
Virtio_switch::add_monitor_port(Port_iface *port)
{
  if (!_monitor)
    {
      _monitor = port;
      return true;
    }

  Dbg(Dbg::Port, Dbg::Warn).printf("'%s' already defined as monitor port,"
                                   " rejecting monitor port '%s'\n",
                                   _monitor->get_name(), port->get_name());
  return false;
}

void
Virtio_switch::check_ports()
{
  for (unsigned idx = 0; idx < _max_used; ++idx)
    {
      Port_iface *port = _ports[idx];
      if (port && port->is_gone())
        {
          Dbg(Dbg::Port, Dbg::Info)
            .printf("Client on port %p has gone. Deleting...\n", port);

          _ports[idx] = nullptr;
          if (idx == _max_used-1)
            --_max_used;

          _mac_table.flush(port);
          delete(port);
        }
    }

  if (_monitor && _monitor->is_gone())
    {
      delete(_monitor);
      _monitor = nullptr;
    }
}

template<typename REQ>
void
Virtio_switch::handle_tx_request(Port_iface *port, REQ const &request)
{
  // Trunk ports are required to have a VLAN tag and only accept packets that
  // belong to a configured VLAN.
  if (port->is_trunk() && !port->match_vlan(request.vlan_id()))
    return; // Drop packet.

  // Access ports must not be VLAN tagged to prevent double tagging attacks.
  if (port->is_access() && request.has_vlan())
    return;  // Drop packet.

  auto handle_request = [](Port_iface *dst_port, Port_iface *src_port,
                           REQ const &req)
    {
      auto transfer_src = req.transfer_src();
      dst_port->handle_request(src_port, transfer_src);
    };

  Mac_addr src = request.src_mac();

  auto dst = request.dst_mac();
  bool is_broadcast = dst.is_broadcast();
  uint16_t vlan = request.has_vlan() ? request.vlan_id() : port->get_vlan();
  _mac_table.learn(src, port, vlan);
  if (L4_LIKELY(!is_broadcast))
    {
      auto *target = _mac_table.lookup(dst, vlan);
      if (target)
        {
          // Do not send packets to the port they came in; they might
          // be sent to us by another switch which does not know how
          // to reach the target.
          if (target != port)
            {
              handle_request(target, port, request);
              if (_monitor && !filter_request(request))
                handle_request(_monitor, port, request);
            }
          return;
        }
    }

  // It is either a broadcast or an unknown destination - send to all
  // known ports except the source port
  for (unsigned idx = 0; idx < _max_used && _ports[idx]; ++idx)
    {
      auto *target = _ports[idx];
      if (target != port && target->match_vlan(vlan))
        handle_request(target, port, request);
    }

  // Send a copy to the monitor port
  if (_monitor && !filter_request(request))
    handle_request(_monitor, port, request);
}

template<typename PORT>
void
Virtio_switch::handle_tx_requests(PORT *port, unsigned &num_reqs_handled)
{
  while (auto req = port->get_tx_request())
    {
      req->dump_request(port);
      handle_tx_request(port, *req);

      if (++num_reqs_handled >= Tx_burst)
        // Port has hit its TX burst limit.
        break;
    }
}

bool
Virtio_switch::handle_l4virtio_port_tx(L4virtio_port *port)
{
  /* handle IRQ on one port for the time being */
  if (!port->tx_work_pending())
    Dbg(Dbg::Port, Dbg::Debug)
      .printf("%s: Irq without pending work\n", port->get_name());

  unsigned num_reqs_handled = 0;
  do
    {
      port->tx_q()->disable_notify();
      port->rx_q()->disable_notify();

      if (num_reqs_handled >= Tx_burst)
        {
          Dbg(Dbg::Port, Dbg::Debug)
            .printf(
              "%s: Tx burst limit hit, reschedule remaining Tx work.\n",
              port->get_name());

          // Port has hit its TX burst limit, so for fairness reasons, stop
          // processing TX work from this port, and instead reschedule the
          // pending work for later.
          port->reschedule_pending_tx();
          // NOTE: Notifications for this port remain disabled, until eventually
          // the reschedule handler calls `handle_l4virtio_port_tx` again.
          return false;
        }

      // Within the loop, to trigger before enabling notifications again.
      all_rx_notify_disable_and_remember();

      try
        {
          // throws Bad_descriptor exceptions raised on SRC port
          handle_tx_requests(port, num_reqs_handled);
        }
      catch (L4virtio::Svr::Bad_descriptor &e)
        {
            Dbg(Dbg::Port, Dbg::Warn, "REQ")
              .printf("%s: caught bad descriptor exception: %s - %i"
                      " -- Signal device error on device %p.\n",
                      __PRETTY_FUNCTION__, e.message(), e.error, port);
            port->device_error();
            all_rx_notify_emit_and_enable();
            return false;
        }

      all_rx_notify_emit_and_enable();

      port->tx_q()->enable_notify();
      port->rx_q()->enable_notify();

      L4virtio::wmb();
      L4virtio::rmb();
    }
  while (port->tx_work_pending());

  return true;
}

#if CONFIG_VNS_IXL
bool
Virtio_switch::handle_ixl_port_tx(Ixl_port *port)
{
  unsigned num_reqs_handled = 0;

  all_rx_notify_disable_and_remember(); 
  handle_tx_requests(port, num_reqs_handled);
  all_rx_notify_emit_and_enable();

  if (num_reqs_handled >= Tx_burst && port->tx_work_pending())
    {
      Dbg(Dbg::Port, Dbg::Info)
        .printf("%s: Tx burst limit hit, reschedule remaining Tx work.\n",
                port->get_name());

      // Port has hit its TX burst limit, so for fairness reasons, stop
      // processing TX work from this port, and instead reschedule the
      // pending work for later.
      port->reschedule_pending_tx();
      return false;
    }

  return true;
}
#endif

