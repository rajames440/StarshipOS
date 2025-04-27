/*
 * Copyright (C) 2020, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *            Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Convenient class for DMA-able memory used for in/out operations.
 */

#pragma once

#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/shared_cap>
#include <l4/re/util/unique_cap>
#include <l4/cxx/ref_ptr>

#include <stdio.h>
#include <cassert>
#include <cstring>

class Inout_buffer : public cxx::Ref_obj
{
public:
  Inout_buffer(char const *cap_name, l4_size_t size,
               L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
               L4Re::Dma_space::Direction dir,
               L4Re::Rm::Flags flags = L4Re::Rm::Flags(0))
  : _size(size), _dma(dma), _paddr(0), _dir(dir)
  {
    auto *e = L4Re::Env::env();

    if (cap_name)
      {
        // use provided capability instead of calling the allocator
        auto ds = e->get_cap<L4Re::Dataspace>(cap_name);
        if (ds.is_valid() && ds->size() == size)
          {
            attach_and_dma_map(size, dir, ds, flags);
            return;
          }

        printf("Capability '%s' not found -- allocating buffer.\n", cap_name);
      }

    _ds = L4Re::chkcap(L4Re::Util::make_unique_cap<L4Re::Dataspace>(),
                       "Allocate dataspace capability for IO memory.");

    L4Re::chksys(e->mem_alloc()->alloc(size, _ds.get(),
                                       L4Re::Mem_alloc::Continuous
                                       | L4Re::Mem_alloc::Pinned),
                 "Allocate pinned memory.");

    attach_and_dma_map(size, dir, _ds.get(), flags);
  }

  Inout_buffer(Inout_buffer const &) = delete;
  Inout_buffer(Inout_buffer &&) = delete;
  Inout_buffer &operator=(Inout_buffer &&rhs) = delete;

  ~Inout_buffer()
  {
    if (_paddr)
      unmap();
  }

  void attach_and_dma_map(l4_size_t size, L4Re::Dma_space::Direction dir,
                          L4::Cap<L4Re::Dataspace> ds, L4Re::Rm::Flags flags)
  {
    auto *e = L4Re::Env::env();

    L4Re::chksys(e->rm()->attach(&_region, size,
                                 L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW | flags,
                                 L4::Ipc::make_cap_rw(ds), 0, L4_PAGESHIFT),
                 "Attach IO memory.");

    l4_size_t out_size = size;
    L4Re::chksys(_dma->map(L4::Ipc::make_cap_rw(ds), 0,
                           &out_size, L4Re::Dma_space::Attributes::None,
                           dir, &_paddr),
                 "Lock memory region for DMA.");
    if (out_size < size)
      L4Re::throw_error(-L4_ENOMEM, "Mapping whole region into DMA space");

    if (_paddr >= 0x1'0000'0000ULL)
      printf("\033[31;1mInout buffer phys at %08llx\033[m\n", _paddr);
  }

  void unmap()
  {
    L4Re::chksys(
      _dma->unmap(_paddr, _size, L4Re::Dma_space::Attributes::None, _dir),
      "Unmap region from DMA.");
    _paddr = 0;
  }

  void dump(char const *name, unsigned item_size, size_t size) const
  {
    printf("%s\n", name);
    if (item_size == 4)
      {
        auto *u32_buf = reinterpret_cast<l4_uint32_t const*>(_region.get());
        for (size_t i = 0; i < size / 4; ++i)
          {
            printf(" %08x", u32_buf[i]);
            if (!((i + 1) % 4) || (i + 1) >= size / 4)
              printf("\n");
          }
      }
    else
      {
        auto *u8_buf = reinterpret_cast<l4_uint8_t const*>(_region.get());
        for (size_t i = 0; i < size; ++i)
          {
            printf("%02x ", u8_buf[i]);
            if (!((i + 1) % 16))
              printf("\n");
          }
      }
  }

  template <class T>
  T *get(unsigned offset = 0) const
  { return reinterpret_cast<T *>(_region.get() + offset); }

  l4_uint64_t pget(unsigned offset = 0) const
  { return _paddr + offset; }

  l4_size_t size() const
  { return _size; }

private:
  l4_size_t _size;
  L4Re::Util::Unique_cap<L4Re::Dataspace> _ds;
  L4Re::Util::Shared_cap<L4Re::Dma_space> _dma;
  L4Re::Rm::Unique_region<char *> _region;
  L4Re::Dma_space::Dma_addr _paddr;
  L4Re::Dma_space::Direction _dir;
};
