/*
 * Copyright (C) 2016-2017, 2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#include "filter.h"

/* This is an example filter and therefore rather verbose. A real
   filter would not produce any output */

bool
filter(const uint8_t *buf, size_t size)
{
  // Packet large enough to apply filter condition?
  if (size <= 13)
    return false;

  uint16_t ether_type = (uint16_t)*(buf + 12) << 8
                      | (uint16_t)*(buf + 13);
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
    printf("%s\n", protocol);
  else
    printf("%04x\n", ether_type);

  if (ether_type == 0x0806)
    {
      printf("Do not filter arp\n");
      return false;
    }

  return true;
}
