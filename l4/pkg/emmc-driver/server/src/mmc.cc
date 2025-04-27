/*
 * Copyright (C) 2020, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <cstdio>

#include "mmc.h"

namespace Emmc {

void Mmc::Reg_csd::dump() const
{
  for (unsigned i = 0; i < sizeof(*this); ++i)
    {
      printf("%02x ", ((l4_uint8_t const *)this)[i]);
      if (!((i + 1) % 16))
        printf("\n");
    }
}

void Mmc::Reg_ecsd::dump() const
{
  for (unsigned i = 0; i < sizeof(*this); ++i)
    {
      printf("%02x ", ((l4_uint8_t const *)this)[i]);
      if (!((i + 1) % 16))
        printf("\n");
    }
}

} // namespace Emmc
