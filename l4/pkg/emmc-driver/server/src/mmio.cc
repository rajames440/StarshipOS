/*
 * Copyright (C) 2021, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "debug.h"
#include "mmio.h"

namespace Hw {

l4_uint64_t
Mmio_space_register_block_base::do_read(l4_addr_t addr, char log2_size) const
{
  l4_uint64_t v;
  if (L4_UNLIKELY(_mmio_space->mmio_read(addr, log2_size, &v) < 0))
    {
      Dbg(Dbg::Warn, "mmio").printf("Could not read from offset %08lx", addr);
      L4Re::throw_error(-L4_EIO, "Read register from MMIO space");
    }
  return v;
}

void
Mmio_space_register_block_base::do_write(l4_uint64_t v,
                                         l4_addr_t addr, char log2_size) const
{
  if (L4_UNLIKELY(_mmio_space->mmio_write(addr, log2_size, v) < 0))
    {
      Dbg(Dbg::Warn, "mmio").printf("Could not write %08llx to offset %08lx",
                                    v, addr);
      L4Re::throw_error(-L4_EIO, "Write register to MMIO space");
    }

  return;
}

} // namespace Hw
