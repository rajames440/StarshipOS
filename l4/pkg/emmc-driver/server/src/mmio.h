/*
 * Copyright (C) 2021, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/drivers/hw_mmio_register_block>
#include <l4/sys/cxx/ipc_types>
#include <l4/re/mmio_space>

#include "iomem.h"

namespace Hw {

/**
 * An MMIO block with 32 bit registers and little endian byte order.
 */
class Mmio_space_register_block_base
{
protected:
  L4::Cap<L4Re::Mmio_space> _mmio_space;
  l4_uint64_t _phys;
  l4_addr_t _shift;

public:
  explicit Mmio_space_register_block_base(L4::Cap<L4Re::Mmio_space> mmio_space,
                                          l4_uint64_t phys, l4_uint64_t,
                                          l4_addr_t shift = 0)
  : _mmio_space(mmio_space), _phys(phys), _shift(shift) {}

  template< typename T >
  T read(l4_addr_t reg) const
  { return do_read(_phys + (reg << _shift), log2_size((T)0)); }

  template< typename T >
  void write(T value, l4_addr_t reg) const
  { do_write(value, _phys + (reg << _shift), log2_size((T)0)); }

  void set_phys(l4_uint64_t phys) { _phys = phys; }
  void set_shift(l4_addr_t shift) { _shift = shift; }

private:
  l4_uint64_t do_read(l4_addr_t addr, char log2_size) const;
  void do_write(l4_uint64_t v, l4_addr_t addr, char log2_size) const;

  static constexpr char log2_size(l4_uint8_t)  { return 0; }
  static constexpr char log2_size(l4_uint16_t) { return 1; }
  static constexpr char log2_size(l4_uint32_t) { return 2; }
  static constexpr char log2_size(l4_uint64_t) { return 3; }
};

template< unsigned MAX_BITS = 32 >
struct Mmio_space_register_block
: L4drivers::Register_block_impl<Mmio_space_register_block<MAX_BITS>, MAX_BITS>,
  Mmio_space_register_block_base
{
  explicit Mmio_space_register_block(L4::Cap<L4Re::Mmio_space> cap,
                                     l4_uint64_t base, l4_uint64_t size,
                                     l4_addr_t shift = 0)
  : Mmio_space_register_block_base(cap, base, size, shift) {}
};

template< unsigned MAX_BITS = 32 >
struct Mmio_map_register_block
: L4drivers::Mmio_register_block<MAX_BITS>
{
  explicit Mmio_map_register_block(L4::Cap<L4Re::Dataspace> iocap,
                                   l4_uint64_t base, l4_uint64_t size,
                                   l4_addr_t shift = 0)
  : iomem(base, size, iocap)
  {
    this->set_base(iomem.vaddr.get() + iomem.offset);
    this->set_shift(shift);
  }
  Iomem iomem;
};

} // namespace Hw
