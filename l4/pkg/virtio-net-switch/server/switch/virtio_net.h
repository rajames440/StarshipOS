/*
 * Copyright (C) 2016-2017, 2019, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/re/dataspace>
#include <l4/re/util/unique_cap>

#include <l4/sys/cxx/ipc_epiface>

#include <l4/l4virtio/server/virtio>
#include <l4/l4virtio/server/l4virtio>
#include <l4/l4virtio/l4virtio>

#include "debug.h"
/**
 * \ingroup virtio_net_switch
 * \{
 */
class Virtqueue : public L4virtio::Svr::Virtqueue
{
public:
  bool kick_queue()
  {
    if (no_notify_guest())
      return false;

    if (_do_kick)
      return true;

    _kick_pending = true;
    return false;
  }

  bool kick_enable_get_pending()
  {
    _do_kick = true;
    return _kick_pending;
  }

  void kick_disable_and_remember()
  {
    _do_kick = false;
    _kick_pending = false;
  }

private:
  bool _do_kick = true;
  bool _kick_pending = false;
};

/**
 * The Base class of a Port.
 *
 * This class provides the Virtio network protocol specific implementation
 * aspects of a port.
 *
 * `Virtio_net` comprises the virtqueues for both, the incoming and the
 * outgoing network requests:
 *
 * - The transmission queue, containing requests to be transmitted to other
 *   ports.  The transmission queue is filled by the client, this port relates
 *   to.
 * - The receive queue, containing requests that have been transmitted from
 *   other ports. The receive queue is filled by the switch.
 */
class Virtio_net :
  public L4virtio::Svr::Device,
  public L4::Epiface_t<Virtio_net, L4virtio::Device>
{
public:
  struct Hdr_flags
  {
    l4_uint8_t raw;
    CXX_BITFIELD_MEMBER( 0, 0, need_csum, raw);
    CXX_BITFIELD_MEMBER( 1, 1, data_valid, raw);
  };

  struct Hdr
  {
    Hdr_flags flags;
    l4_uint8_t gso_type;
    l4_uint16_t hdr_len;
    l4_uint16_t gso_size;
    l4_uint16_t csum_start;
    l4_uint16_t csum_offset;
    l4_uint16_t num_buffers;
  };

  struct Features : L4virtio::Svr::Dev_config::Features
  {
    Features() = default;
    Features(l4_uint32_t raw) : L4virtio::Svr::Dev_config::Features(raw) {}

    CXX_BITFIELD_MEMBER( 0,  0, csum, raw);       // host handles partial csum
    CXX_BITFIELD_MEMBER( 1,  1, guest_csum, raw); // guest handles partial csum
    CXX_BITFIELD_MEMBER( 5,  5, mac, raw);        // host has given mac
    CXX_BITFIELD_MEMBER( 6,  6, gso, raw);        // host handles packets /w any GSO
    CXX_BITFIELD_MEMBER( 7,  7, guest_tso4, raw); // guest handles TSOv4 in
    CXX_BITFIELD_MEMBER( 8,  8, guest_tso6, raw); // guest handles TSOv6 in
    CXX_BITFIELD_MEMBER( 9,  9, guest_ecn, raw);  // guest handles TSO[6] with ECN in
    CXX_BITFIELD_MEMBER(10, 10, guest_ufo, raw);  // guest handles UFO in
    CXX_BITFIELD_MEMBER(11, 11, host_tso4, raw);  // host handles TSOv4 in
    CXX_BITFIELD_MEMBER(12, 12, host_tso6, raw);  // host handles TSOv6 in
    CXX_BITFIELD_MEMBER(13, 13, host_ecn, raw);   // host handles TSO[6] with ECN in
    CXX_BITFIELD_MEMBER(14, 14, host_ufo, raw);   // host handles UFO
    CXX_BITFIELD_MEMBER(15, 15, mrg_rxbuf, raw);  // host can merge receive buffers
    CXX_BITFIELD_MEMBER(16, 16, status, raw);     // virtio_net_config.status available
    CXX_BITFIELD_MEMBER(17, 17, ctrl_vq, raw);    // Control channel available
    CXX_BITFIELD_MEMBER(18, 18, ctrl_rx, raw);    // Control channel RX mode support
    CXX_BITFIELD_MEMBER(19, 19, ctrl_vlan, raw);  // Control channel VLAN filtering
    CXX_BITFIELD_MEMBER(20, 20, ctrl_rx_extra, raw); // Extra RX mode control support
    CXX_BITFIELD_MEMBER(21, 21, guest_announce, raw); // Guest can announce device on the network
    CXX_BITFIELD_MEMBER(22, 22, mq, raw);         // Device supports Receive Flow Steering
    CXX_BITFIELD_MEMBER(23, 23, ctrl_mac_addr, raw); // Set MAC address
  };

  enum
  {
    Rx = 0,
    Tx = 1,
  };

  struct Net_config_space
  {
    // The config defining mac address (if VIRTIO_NET_F_MAC aka Features::mac)
    l4_uint8_t mac[6];
    // currently not used ...
    l4_uint16_t status;
    l4_uint16_t max_virtqueue_pairs;
  };

  L4virtio::Svr::Dev_config_t<Net_config_space> _dev_config;

  explicit Virtio_net(unsigned vq_max)
  : L4virtio::Svr::Device(&_dev_config),
    _dev_config(L4VIRTIO_VENDOR_KK, L4VIRTIO_ID_NET, 2),
    _vq_max(vq_max)
  {
    Features hf(0);
    hf.ring_indirect_desc() = true;
    hf.mrg_rxbuf() = true;
#if 0
    // disable currently unsupported options, but leave them in for
    // documentation purposes
    hf.csum()      = true;
    hf.host_tso4() = true;
    hf.host_tso6() = true;
    hf.host_ufo()  = true;
    hf.host_ecn()  = true;

    hf.guest_csum() = true;
    hf.guest_tso4() = true;
    hf.guest_tso6() = true;
    hf.guest_ufo()  = true;
    hf.guest_ecn()  = true;
#endif

    _dev_config.host_features(0) = hf.raw;
    _dev_config.set_host_feature(L4VIRTIO_FEATURE_VERSION_1);
    _dev_config.reset_hdr();

    reset_queue_config(Rx, vq_max);
    reset_queue_config(Tx, vq_max);
  }

  void reset() override
  {
    for (L4virtio::Svr::Virtqueue &q: _q)
      q.disable();

    reset_queue_config(Rx, _vq_max);
    reset_queue_config(Tx, _vq_max);
    _dev_config.reset_hdr();
  }

  template<typename T, unsigned N >
  static unsigned array_length(T (&)[N]) { return N; }

  int reconfig_queue(unsigned index) override
  {
    Dbg(Dbg::Virtio, Dbg::Info, "Virtio")
      .printf("(%p): Reconfigure queue %d (%p): Status: %02x\n",
              this, index, _q + index, _dev_config.status().raw);

    if (index >= array_length(_q))
      return -L4_ERANGE;

    if (setup_queue(_q + index, index, _vq_max))
      return 0;

    return -L4_EINVAL;
  }

  void dump_features(Dbg const &dbg, const volatile l4_uint32_t *p)
  {
    dbg.cprintf("%08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n",
                p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[17]);
  }

  void dump_features()
  {
    Dbg info(Dbg::Virtio, Dbg::Info, "Virtio");
    if (!info.is_active())
      return;

    auto *hdr = _dev_config.hdr();

    info.printf("Device %p running (%02x)\n\thost features:  ",
                this, _dev_config.status().raw);
    dump_features(info, hdr->dev_features_map);
    info.printf("\tguest features: ");
    dump_features(info, hdr->driver_features_map);
  }

  bool check_features() override
  {
    _negotiated_features = _dev_config.negotiated_features(0);
    return true;
  }

  bool device_needs_reset() const
  { return _dev_config.status().device_needs_reset(); }

  /** Check whether both virtqueues are ready. */
  bool check_queues() override
  {
    for (L4virtio::Svr::Virtqueue &q: _q)
      if (!q.ready())
        {
          reset();
          Err().printf("failed to start queues\n");
          return false;
        }
    dump_features();
    return true;
  }

  Server_iface *server_iface() const override
  { return L4::Epiface::server_iface(); }

  /**
   * Save the `_kick_guest_irq` that the client sent via
   * `device_notification_irq()`.
   */
  void register_single_driver_irq() override
  {
    _kick_guest_irq = L4Re::Util::Unique_cap<L4::Irq>(
        L4Re::chkcap(server_iface()->template rcv_cap<L4::Irq>(0)));
    L4Re::chksys(server_iface()->realloc_rcv_cap(0));
  }

  void trigger_driver_config_irq() override
  {
    _dev_config.add_irq_status(L4VIRTIO_IRQ_STATUS_CONFIG);
    _kick_guest_irq->trigger();
  }

  /**
   * Trigger  the `_kick_guest_irq` IRQ.
   *
   * This function gets called on the receiving port, when a request was
   * successfully transmitted by the switch.
   */
  void notify_queue(L4virtio::Svr::Virtqueue *queue)
  {
    // Downcast to Virtqueue to access kick_queue() - we know that our
    // queues have the type Virtqueue.
    Virtqueue *q = static_cast<Virtqueue*>(queue);
    if (q->kick_queue())
      {
        _dev_config.add_irq_status(L4VIRTIO_IRQ_STATUS_VRING);
        _kick_guest_irq->trigger();
      }
  }

  void kick_emit_and_enable()
  {
    bool kick_pending = false;

    for (auto &q : _q)
      kick_pending |= q.kick_enable_get_pending();

    if (kick_pending)
      {
        _dev_config.add_irq_status(L4VIRTIO_IRQ_STATUS_VRING);
        _kick_guest_irq->trigger();
      }
  }

  void kick_disable_and_remember()
  {
    for (auto &q : _q)
      q.kick_disable_and_remember();
  }

  Features negotiated_features() const
  { return _negotiated_features; }

  /** Getter for the transmission queue. */
  Virtqueue *tx_q() { return &_q[Tx]; }
  /** Getter for the receive queue. */
  Virtqueue *rx_q() { return &_q[Rx]; }
  /** Getter for the transmission queue. */
  Virtqueue const *tx_q() const { return &_q[Tx]; }
  /** Getter for the receive queue. */
  Virtqueue const *rx_q() const { return &_q[Rx]; }

private:
  Features _negotiated_features;
  /** Maximum number of entries in a virtqueue that is used by the port */
  unsigned _vq_max;
  /** the two used virtqueues */
  Virtqueue _q[2];
  /**
   * The IRQ used to notify the associated client that a new network request
   * has been received and is present in the receive queue.
   */
  L4Re::Util::Unique_cap<L4::Irq> _kick_guest_irq;
};
/**\}*/
