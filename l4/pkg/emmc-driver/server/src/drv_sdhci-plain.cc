/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "drv_sdhci.h"
#include "drv_sdhci-impl.h"
#include "factory.h"

namespace Emmc {

template <>
void
Sdhci<Sdhci_type::Plain>::init_platform(L4Re::Util::Shared_cap<L4Re::Dma_space> const &)
{}

template <>
void
Sdhci<Sdhci_type::Plain>::done_platform()
{}

} // namespace Emmc

namespace {

using namespace Emmc;

struct F_sdhci_plain : Factory
{
  cxx::Ref_ptr<Device_factory::Device_type>
  create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
         L4::Cap<L4::Icu> icu, L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
         unsigned max_seg, Device_type_disable dt_disable)
  {
    L4::Cap<L4Re::Mmio_space> mmio_space;
    return cxx::make_ref_obj<Device<Sdhci<Sdhci_type::Plain>>>(
             nr, mmio_addr, mmio_size, iocap, mmio_space, irq_num, irq_mode,
             icu, dma, registry, host_clock, max_seg, dt_disable);
  }
};

static F_sdhci_plain f_sdhci_plain;
static Device_type_pci t_sdhci_plain{0x80501, &f_sdhci_plain};

} // namespace
