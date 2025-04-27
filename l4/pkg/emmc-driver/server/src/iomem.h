/*
 * Copyright (C) 2020-2021, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/re/env>
#include <l4/re/error_helper>

/**
 * Self-attaching IO memory.
 */
struct Iomem
{
  L4Re::Rm::Unique_region<l4_addr_t> vaddr;

  Iomem(l4_uint64_t phys_addr, l4_uint64_t size, L4::Cap<L4Re::Dataspace> iocap)
  : offset(phys_addr & ~L4_PAGEMASK)
  {
    auto *e = L4Re::Env::env();
    L4Re::chksys(e->rm()->attach(&vaddr, size,
                                 L4Re::Rm::F::Search_addr
                                 | L4Re::Rm::F::Cache_uncached
                                 | L4Re::Rm::F::RW
                                 | L4Re::Rm::F::Eager_map,
                                 L4::Ipc::make_cap_rw(iocap),
                                 phys_addr, L4_PAGESHIFT),
                 "Attach in/out buffer.");
  }

  l4_addr_t offset;
};
