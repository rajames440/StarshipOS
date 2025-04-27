/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/mbox-bcm2835/mbox.h>

#include "drv_sdhci.h"
#include "drv_sdhci-impl.h"
#include "factory.h"

namespace Emmc {

template <>
void
Sdhci<Sdhci_type::Bcm2711>::init_platform(L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma)
{
  auto *e = L4Re::Env::env();
  auto vbus = L4Re::chkcap(e->get_cap<L4vbus::Vbus>("vbus_mbox"),
                           "Get 'vbus' capability for mbox device.",
                           -L4_ENOENT);

  static Dbg log{1, "mbox"};
  bcm2835_mbox = new Bcm2835_mbox(vbus, log, dma);

  /* See https://github.com/raspberrypi/linux/commit/
   *     3d2cbb64483691c8f8cf88e17d7d581d9402ac4b
   * ``emmc2 has different DMA constraints based on SoC revisions...
   * The firmware will find whether the emmc2bus alias is defined, and if so,
   * it'll edit the dma-ranges property below accordingly.´´
   *
   * Well, that's not possible here.
   * See https://github.com/raspberrypi/documentation/blob/develop/
   *     documentation/asciidoc/computers/raspberry-pi/revision-codes.adoc
   *
   * Older boards:
   *  - The EMMC2 bus can only directly address the first 1GB.
   *    XXX Is that true?
   *  - The PCIe interface can only directly address the first 3GB.
   *  - Device tree for emmc2bus:
   *      #address-cells = <0x02>;
   *      #size-cells = <0x01>;
   *      emmc2bus: dma-ranges = <0x00 0xc0000000 0x00 0x00 0x40000000>;
   *
   * Newer boards:
   *  - Device tree for emmc2bus:
   *      #address-cells = <0x02>;
   *      #size-cells = <0x01>;
   *      emmc2bus: dma-ranges = <0x0 0x0 0x0 0x0 0xfc000000>;
   */
  Bcm2835_mbox::Soc_rev board_rev{bcm2835_mbox->get_board_rev()};
  _dma_offset = 0xc0000000UL; // default: assume old revision
  _dma_limit = 0x3fffffffULL;
  if (board_rev.new_style())
    {
      // Try to detect the C0 stepping
      switch (board_rev.type())
        {
        case 0x11: // 4B
          if (board_rev.revision() <= 2)
            break;
          _dma_offset = 0UL; // new revision
          _dma_limit = 0xffffffffULL; // XXX is this correct?
          break;
        case 0x13: // 400
          _dma_offset = 0UL; // new revision
          _dma_limit = 0xffffffffULL; // XXX is this correct?
          break;
        }
    }

  l4_uint64_t memsize;
  switch (board_rev.memory_size())
    {
    case 0: memsize = 256 << 20; break;
    case 1: memsize = 512 << 20; break;
    case 2: memsize = 1ULL << 30; break;
    case 3: memsize = 2ULL << 30; break;
    case 4: memsize = 4ULL << 30; break;
    case 5: memsize = 8ULL << 30; break;
    default: memsize = 0; break;
    }
  printf("RAM: %s, Revision: %08x => \033[31;1mDMA offset = %08lx\033[m.\n",
         memsize ? Util::readable_size(memsize).c_str() : "unknown",
         board_rev.raw, _dma_offset);
}

template <>
void
Sdhci<Sdhci_type::Bcm2711>::done_platform()
{
  if (bcm2835_mbox)
    delete bcm2835_mbox;
}

template <>
void
Sdhci<Sdhci_type::Bcm2711>::set_voltage_18(bool enable_18v)
{
  l4_uint32_t value = enable_18v ? 1 : 0;
  printf("SET_VOLTAGE_18: enable = %d\n", enable_18v);
  bcm2835_mbox->set_fw_gpio(Bcm2835_mbox::Raspi_exp_gpio_vdd_sd_io_sel, value);
}

}; // namespace Emmc

namespace {

using namespace Emmc;

struct F_sdhci_iproc : Factory
{
  cxx::Ref_ptr<Device_factory::Device_type>
  create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
         L4::Cap<L4::Icu> icu, L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
         unsigned max_seg, Device_type_disable dt_disable) override
  {
    L4::Cap<L4Re::Mmio_space> mmio_space;
    return cxx::make_ref_obj<Device<Sdhci<Sdhci_type::Bcm2711>>>(
             nr, mmio_addr, mmio_size, iocap, mmio_space, irq_num, irq_mode,
             icu, dma, registry, host_clock, max_seg, dt_disable);
  }

  l4_uint32_t guess_clock(l4_uint64_t mmio_addr) override
  {
    switch (mmio_addr)
      {
      case 0xfe340000: return 100000000;
      default:         return 0;
      }
  }
};

static F_sdhci_iproc f_sdhci_iproc;
static Device_type_nopci t_sdhci_iproc{"brcm,bcm2711-emmc2", &f_sdhci_iproc};

} // namespace
