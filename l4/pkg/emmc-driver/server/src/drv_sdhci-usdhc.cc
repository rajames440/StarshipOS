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
Sdhci<Sdhci_type::Usdhc>::init_platform(L4Re::Util::Shared_cap<L4Re::Dma_space> const &)
{}

template <>
void
Sdhci<Sdhci_type::Usdhc>::done_platform()
{}

} // namespace Emmc

namespace {

using namespace Emmc;

struct F_sdhci_usdhc : Factory
{
  cxx::Ref_ptr<Device_factory::Device_type>
  create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
         L4::Cap<L4::Icu> icu, L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
         unsigned max_seg, Device_type_disable dt_disable) override
  {
    L4::Cap<L4Re::Mmio_space> mmio_space;
    return cxx::make_ref_obj<Device<Sdhci<Sdhci_type::Usdhc>>>(
             nr, mmio_addr, mmio_size, iocap, mmio_space, irq_num, irq_mode,
             icu, dma, registry, host_clock, max_seg, dt_disable);
  }

  l4_uint32_t guess_clock(l4_uint64_t mmio_addr) override
  {
    switch (mmio_addr)
      {
      case 0x30b40000: return 400000000;
      case 0x30b50000: return 200000000;
      case 0x30b60000: return 200000000;
      case 0x5b010000: return 396000000;
      case 0x5b020000: return 198000000;
      case 0x5b030000: return 198000000;
      case 0x402f0000: return 400000000; // s32g2-usdhc
      default:
        L4Re::throw_error(-L4_EINVAL, "Unknown host clock");
      }
  }
};

static F_sdhci_usdhc f_sdhci_usdhc;
static Device_type_nopci t_sdhci_usdhc_a{"fsl,imx8mq-usdhc", &f_sdhci_usdhc};
static Device_type_nopci t_sdhci_usdhc_b{"fsl,imx8qm-usdhc", &f_sdhci_usdhc};
static Device_type_nopci t_sdhci_usdhc_c{"fsl,imx7d-usdhc", &f_sdhci_usdhc};
static Device_type_nopci t_sdhci_usdhc_d{"fsl,s32gen1-usdhc", &f_sdhci_usdhc};
static Device_type_nopci t_sdhci_usdhc_e{"nxp,s32g2-usdhc", &f_sdhci_usdhc};

} // namespace
