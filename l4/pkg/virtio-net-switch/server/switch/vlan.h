/*
 * Copyright (C) 2020, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jan Klötzke <jan.kloetzke@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/cxx/minmax>
#include <l4/l4virtio/server/virtio>
#include <l4/sys/types.h>
#include <string.h>

#include "virtio_net.h"
#include "virtio_net_buffer.h"

namespace {

const l4_uint16_t VLAN_ID_NATIVE = 0xffffU; ///< Pseudo ID for native ports
const l4_uint16_t VLAN_ID_TRUNK = 0xfffeU;  ///< Pseudo ID for trunk ports

inline bool vlan_valid_id(l4_uint16_t id)
{
  return id > 0U && id < 0xfffU;
}

}
/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * Class for VLAN packet rewriting.
 */
class Virtio_vlan_mangle
{
  l4_uint16_t _tci;
  l4_uint8_t _mac_remaining;
  l4_int8_t _tag_remaining;

  constexpr Virtio_vlan_mangle(l4_uint16_t tci, l4_int8_t tag_remaining)
  : _tci{tci}, _mac_remaining{12}, _tag_remaining{tag_remaining}
  {}

public:
  /**
   * Default constructor.
   *
   * The packet is not touched in any way.
   */
  Virtio_vlan_mangle()
  : _tci{0}, _mac_remaining{0}, _tag_remaining{0}
  {}

  /**
   * Construct an object that adds a VLAN tag.
   *
   * \param tci The TCI field of the VLAN tag to add.
   *
   * It is the callers responsibility to ensure that the packet is not already
   * tagged.
   */
  static constexpr Virtio_vlan_mangle add(l4_uint16_t tci)
  {
    return Virtio_vlan_mangle(tci, 4);
  }

  /**
   * Construct an object that removes the VLAN tag.
   *
   * This object assumes that the Ethernet packet has a VLAN tag and will
   * slavishly remove the necessary bytes from the packet.
   */
  static constexpr Virtio_vlan_mangle remove()
  {
    return Virtio_vlan_mangle(0xffffU, -4);
  }

  /**
   * Copy packet from \a src to \a dst.
   *
   * \param src Source packet buffer
   * \param dst Destination packet buffer
   * \return The number of bytes copied
   *
   * Copy the data from \a src to \a dst, possibly rewriting parts of the
   * packet. The method is expected to be called repeatedly until the source
   * packet is finished. Partial copies are allowed (including reading nothing
   * from the source buffer) as long as progress is made, i.e. repeatedly
   * calling this function eventually consumes the source buffer.
   */
  l4_uint32_t copy_pkt(Buffer &dst, Buffer &src)
  {
    l4_uint32_t ret;

    if (L4_LIKELY(_tci == 0))
      {
        // pass through (no tag or keep tag)
        ret = src.copy_to(&dst);
      }
    else if (_mac_remaining)
      {
        // copy initial MAC addresses
        ret = src.copy_to(&dst, _mac_remaining);
        _mac_remaining -= ret;
      }
    else if (_tag_remaining > 0)
      {
        // add VLAN tag
        l4_uint8_t tag[4] = {
          0x81, 0x00,
          static_cast<l4_uint8_t>(_tci >> 8),
          static_cast<l4_uint8_t>(_tci & 0xffU)
        };

        ret = cxx::min(static_cast<l4_uint32_t>(_tag_remaining), dst.left);
        memcpy(dst.pos, &tag[4 - _tag_remaining], ret);
        dst.skip(ret);
        _tag_remaining -= (int)ret;
      }
    else if (_tag_remaining < 0)
      {
        // remove VLAN tag
        _tag_remaining += static_cast<int>(src.skip(-_tag_remaining));
        ret = 0;
      }
    else
      ret = src.copy_to(&dst);

    return ret;
  }

  /**
   * Rewrite the virtio network header.
   *
   * \param hdr The virtio header of the packet
   *
   * This method is called exactly once for every virtio network packet. Any
   * necessary changes to the header are done in-place.
   */
  void rewrite_hdr(Virtio_net::Hdr *hdr)
  {
    if (L4_UNLIKELY(_tci != 0 && hdr->flags.need_csum()))
      {
        if (_tci == 0xffffU)
          hdr->csum_start -= 4U;
        else
          hdr->csum_start += 4U;
      }
  }
};

/**\}*/
