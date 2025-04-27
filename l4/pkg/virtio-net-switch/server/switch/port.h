/*
 * Copyright (C) 2016-2018, 2020, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include "request.h"
#include "mac_addr.h"
#include "vlan.h"

#include <cassert>
#include <set>
#include <vector>

/**
 * \ingroup virtio_net_switch
 * \{
 */

class Port_iface
{
public:
  Port_iface(char const *name)
  {
    strncpy(_name, name, sizeof(_name));
    _name[sizeof(_name) - 1] = '\0';
  }

  // delete copy and assignment
  Port_iface(Port_iface const &) = delete;
  Port_iface &operator = (Port_iface const &) = delete;

  virtual ~Port_iface() = default;

  char const *get_name() const
  { return _name; }

  l4_uint16_t get_vlan() const
  { return _vlan_id; }

  inline bool is_trunk() const
  { return _vlan_id == VLAN_ID_TRUNK; }

  inline bool is_native() const
  { return _vlan_id == VLAN_ID_NATIVE; }

  inline bool is_access() const
  { return !is_trunk() && !is_native(); }

  /**
   * Set port as access port for a certain VLAN.
   *
   * \param id  The VLAN id for traffic on this port (0 < id < 0xfff)
   *
   * The port does not see VLAN tags but belongs to the given VLAN.
   */
  void set_vlan_access(l4_uint16_t id)
  {
    assert(vlan_valid_id(id));
    _vlan_id = id;
    _vlan_bloom_filter = 0;
    _vlan_ids.clear();
  }

  /**
   * Set port as trunk port.
   *
   * \param ids List of VLAN ids that are switched on this port
   *
   * Incoming traffic on this port is expected to have a VLAN tag that matches
   * one in \a ids. Outgoing traffic will be tagged it if there is no tag in
   * the Ethernet header yet.
   */
  void set_vlan_trunk(const std::vector<l4_uint16_t> &ids)
  {
    // bloom filter to quickly reject packets that do not belong to this port
    l4_uint32_t filter = 0;

    _vlan_ids.clear();
    for (const auto id : ids)
      {
        assert(vlan_valid_id(id));
        filter |= vlan_bloom_hash(id);
        _vlan_ids.insert(id);
      }

    _vlan_id = VLAN_ID_TRUNK;
    _vlan_bloom_filter = filter;
  }

  /**
   * This port shall participate in all VLANs.
   */
  void set_vlan_trunk_all()
  {
    _vlan_all = true;
    _vlan_id = VLAN_ID_TRUNK;
    _vlan_bloom_filter = -1;
  }

  /**
   * Set this port as monitor port.
   *
   * Ensures that outgoing traffic will have a VLAN tag if the packet belongs
   * to a VLAN. Packets coming from native ports will remain untagged.
   */
  void set_monitor()
  {
    _vlan_id = VLAN_ID_TRUNK;
    _vlan_bloom_filter = 0;
  }

  /**
   * Match VLAN id.
   *
   * \param id  The VLAN id of the packet or VLAN_ID_NATIVE.
   *
   * Check whether VLAN \a id is switched on this port. Packets of native ports
   * have the special VLAN_ID_NATIVE id.
   */
  bool match_vlan(uint16_t id)
  {
    // Regular case native/access port
    if (id == _vlan_id)
      return true;

    // This port participates in all VLANs
    if (_vlan_all)
      return true;

    // Quick check: does port probably accept this VLAN?
    if ((_vlan_bloom_filter & vlan_bloom_hash(id)) == 0)
      return false;

    return _vlan_ids.find(id) != _vlan_ids.end();
  }

  /**
   * Get MAC address.
   *
   * Might be Mac_addr::Addr_unknown if this port has no explicit MAC address
   * set.
   */
  inline Mac_addr mac() const
  { return _mac; }

  Virtio_vlan_mangle create_vlan_mangle(Port_iface *src_port) const
  {
    Virtio_vlan_mangle mangle;

    if (is_trunk())
      {
        /*
         * Add a VLAN tag only if the packet does not already have one (by
         * coming from another trunk port) or if the packet does not belong to
         * any VLAN (by coming from a native port). The latter case is only
         * relevant if this is a monitor port. Otherwise traffic from native
         * ports is never forwarded to trunk ports.
         */
        if (!src_port->is_trunk() && !src_port->is_native())
          mangle = Virtio_vlan_mangle::add(src_port->get_vlan());
      }
    else
      /*
       * Remove VLAN tag only if the packet actually has one (by coming from a
       * trunk port).
       */
      if (src_port->is_trunk())
        mangle = Virtio_vlan_mangle::remove();

    return mangle;
  }

  virtual void rx_notify_disable_and_remember() = 0;
  virtual void rx_notify_emit_and_enable() = 0;

  virtual bool is_gone() const = 0;

  /** Get one request from the transmission queue */
  // std::optional<Net_request> get_tx_request() = 0;

  enum class Result
  {
    Delivered, Exception, Dropped,
  };

  /**
   * Handle a request, i.e. send the request to this port.
   *
   * \throws L4virtio::Svr::Bad_descriptor  Exception raised in SRC port queue.
   */
  virtual Result handle_request(Port_iface *src_port,
                                Net_transfer &src) = 0;

  void reschedule_pending_tx()
  { _pending_tx_reschedule->trigger(); }

protected:
  /*
   * VLAN related management information.
   *
   * A port may either be
   *  - a native port (_vlan_id == VLAN_ID_NATIVE), or
   *  - an access port (_vlan_id set accordingly), or
   *  - a trunk port (_vlan_id == VLAN_ID_TRUNK, _vlan_bloom_filter and
   *    _vlan_ids populated accordingly, or _vlan_all == true).
   */
  l4_uint16_t _vlan_id = VLAN_ID_NATIVE; // VID for native/access port
  l4_uint32_t _vlan_bloom_filter = 0; // Bloom filter for trunk ports
  std::set<l4_uint16_t> _vlan_ids;  // Authoritative list of trunk VLANs
  bool _vlan_all; // This port participates in all VLANs (ignoring _vlan_ids)

  inline l4_uint32_t vlan_bloom_hash(l4_uint16_t vid)
  { return 1UL << (vid & 31U); }

  /**
   * Reschedule TX request handling for port that hit its TX burst limit.
   */
  L4::Cap<L4::Irq> _pending_tx_reschedule;

  Mac_addr _mac = Mac_addr(Mac_addr::Addr_unknown);  /**< The MAC address of the port. */
  char _name[20]; /**< Debug name */
};

/**\}*/
