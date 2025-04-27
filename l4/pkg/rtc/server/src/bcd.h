/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Andreas Wiese <andreas.wiese@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/sys/l4int.h>

/* convert binary coded decimal to binary */
static inline l4_uint8_t bcd2bin(l4_uint8_t value)
{
  return (value >> 4) * 10 + (value & 0x0f);
}

/* convert binary to binary coded decimal */
static inline l4_uint8_t bin2bcd(l4_uint8_t value)
{
  return (((value / 10) % 10) << 4) | (value % 10);
}
