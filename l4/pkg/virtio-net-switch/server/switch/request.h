/*
 * Copyright (C) 2016-2017, 2020, 2022, 2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include "mac_addr.h"
#include "virtio_net.h"
#include "virtio_net_buffer.h"
#include "vlan.h"

#include <l4/l4virtio/server/virtio>


/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * A network request to only a single destination.
 *
 * A `Net_request` can have multiple destinations (being a broadcast
 * request, for example). That is why it is processed by multiple
 * `Net_transfer`s, each representing the delivery to a single
 * destination port.
 *
 * `Port_iface::handle_request` uses the `Net_transfer` to move one packet to
 * the destination of the request.
 */
class Net_transfer
{
public:
  virtual ~Net_transfer() = default;

  /**
   * Identifier for the underlying `Net_request`, used for logging purposes.
   */
  void const *req_id() const { return _req_id; }

  /**
   * Populate the virtio-net header for the destination.
   */
  virtual void copy_header(Virtio_net::Hdr *dst_header) const = 0;

  /**
   * Buffer containing (a part of) the packet data.
   *
   * Once emptied, a call to `done()` might replenish the buffer, in case the
   * net request consisted of multiple chained buffers.
   */
  Buffer &cur_buf() { return _cur_buf; }

  /**
   * Check whether the transfer has been completed, i.e. the entire packet data
   * has been copied.
   *
   * \retval false  There is remaining packet data that needs to be copied.
   * \retval true   The entire packet data has been copied.
   *
   * \throws L4virtio::Svr::Bad_descriptor  Exception raised in SRC port queue.
   */
  virtual bool done() = 0;

protected:
  Buffer _cur_buf;
  void const *_req_id;
};

class Net_request
{
public:
  /** Get the Mac address of the destination port. */
  Mac_addr dst_mac() const
  {
    return (_pkt.pos && _pkt.left >= Mac_addr::Addr_length)
      ? Mac_addr(_pkt.pos)
      : Mac_addr(Mac_addr::Addr_unknown);
  }

  /** Get the Mac address of the source port. */
  Mac_addr src_mac() const
  {
    return (_pkt.pos && _pkt.left >= Mac_addr::Addr_length * 2)
      ? Mac_addr(_pkt.pos + Mac_addr::Addr_length)
      : Mac_addr(Mac_addr::Addr_unknown);
  }

  bool has_vlan() const
  {
    if (!_pkt.pos || _pkt.left < 14)
      return false;

    uint8_t *p = reinterpret_cast<uint8_t *>(_pkt.pos);
    return p[12] == 0x81U && p[13] == 0x00U;
  }

  uint16_t vlan_id() const
  {
    if (!has_vlan() || _pkt.left < 16)
      return VLAN_ID_NATIVE;

    uint8_t *p = reinterpret_cast<uint8_t *>(_pkt.pos);
    return (uint16_t{p[14]} << 8 | p[15]) & 0xfffU;
  }

  /**
   * Get the location and size of the current buffer.
   *
   * \param[out] size   Size of the current buffer.
   *
   * \return  Address of the current buffer.
   *
   * This function returns the address and size of the currently
   * active buffer for this request. The buffer might only be a part
   * of the request, which may consist of more than one buffer.
   */
  uint8_t const *buffer(size_t *size) const
  {
    *size = _pkt.left;
    return reinterpret_cast<uint8_t const *>(_pkt.pos);
  }

  void dump_pkt() const
  {
    Dbg pkt_debug(Dbg::Packet, Dbg::Debug, "PKT");
    if (pkt_debug.is_active())
      {
        pkt_debug.cprintf("\t");
        src_mac().print(pkt_debug);
        pkt_debug.cprintf(" -> ");
        dst_mac().print(pkt_debug);
        pkt_debug.cprintf("\n");

        Dbg pkt_trace(Dbg::Packet, Dbg::Trace, "PKT");
        if (pkt_trace.is_active() && _pkt.left >= 14)
          {
            uint8_t const *packet = reinterpret_cast<uint8_t const *>(_pkt.pos);
            pkt_trace.cprintf("\n\tEthertype: ");
            uint16_t ether_type = uint16_t{packet[12]} << 8 | packet[13];
            char const *protocol;
            switch (ether_type)
              {
              case 0x0800: protocol = "IPv4"; break;
              case 0x0806: protocol = "ARP"; break;
              case 0x8100: protocol = "Vlan"; break;
              case 0x86dd: protocol = "IPv6"; break;
              case 0x8863: protocol = "PPPoE Discovery"; break;
              case 0x8864: protocol = "PPPoE Session"; break;
              default: protocol = nullptr;
              }
            if (protocol)
              pkt_trace.cprintf("%s\n", protocol);
            else
              pkt_trace.cprintf("%04x\n", ether_type);
          }
      }
  }

protected:
  Buffer _pkt;
};

/**\}*/
