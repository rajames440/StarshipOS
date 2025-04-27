/*
 * Copyright (C) 2016-2017, 2020, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <cstring>
#include <inttypes.h>
/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * A wrapper class around the value of a MAC address.
 */
class Mac_addr
{
public:
  enum
  {
    Addr_length = 6,
    Addr_unknown = 0ULL
  };

  explicit Mac_addr(char const *_src)
  {
    /* A mac address is 6 bytes long, it is transmitted in big endian
       order over the network. For our internal representation we
       focus on easy testability of broadcast/multicast and reorder
       the bytes that the most significant byte becomes the least
       significant one. */
    unsigned char const *src = reinterpret_cast<unsigned char const *>(_src);
    _mac =    ((uint64_t)src[0])        | (((uint64_t)src[1]) << 8)
           | (((uint64_t)src[2]) << 16) | (((uint64_t)src[3]) << 24)
           | (((uint64_t)src[4]) << 32) | (((uint64_t)src[5]) << 40);
  }

  explicit Mac_addr(uint64_t mac) : _mac{mac} {}

  Mac_addr(Mac_addr const &other) : _mac{other._mac} {}

  /** Check if MAC address is a broadcast or multicast address. */
  bool is_broadcast() const
  {
    /* There are broadcast and multicast addresses, both are supposed
       to be delivered to all station and the local network (layer 2).

       Broadcast address is FF:FF:FF:FF:FF:FF, multicast addresses have
       the LSB of the first octet set. Since this holds for both
       broadcast and multicast we test for the multicast bit here.

       In our internal representation we store the bytes in reverse
       order, so we test the least significant bit of the least
       significant byte.
    */
    return _mac & 1;
  }

  /** Check if the MAC address is not yet known. */
  bool is_unknown() const
  { return _mac == Addr_unknown; }

  bool operator == (Mac_addr const &other) const
  { return _mac == other._mac; }

  bool operator != (Mac_addr const &other) const
  { return _mac != other._mac; }

  bool operator < (Mac_addr const &other) const
  { return _mac < other._mac; }

  Mac_addr& operator = (Mac_addr const &other)
  { _mac = other._mac; return *this; }

  Mac_addr& operator = (uint64_t mac)
  { _mac = mac; return *this; }

  template<typename T>
  void print(T &stream) const
  {
    stream.cprintf("%02x:%02x:%02x:%02x:%02x:%02x",
                   (int)(_mac & 0xff)        , (int)((_mac >> 8)  & 0xff),
                   (int)((_mac >> 16) & 0xff), (int)((_mac >> 24) & 0xff),
                   (int)((_mac >> 32) & 0xff), (int)((_mac >> 40) & 0xff));
  }

private:
  /// Mac addresses are 6 bytes long, we use 8 bytes to store them
  uint64_t _mac;
};
/**\}*/
