/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <map>

#include <l4/vbus/vbus_interfaces.h>

#include "debug.h"
#include "factory.h"

namespace Emmc {

static Dbg warn(Dbg::Warn, "factory");
static Dbg info(Dbg::Info, "factory");
static Dbg trace(Dbg::Trace, "factory");

cxx::H_list_t<Device_type_pci> Device_type_pci::types(true);
cxx::H_list_t<Device_type_nopci> Device_type_nopci::types(true);

bool
Factory::nopci_dev(L4vbus::Device const &dev, l4vbus_device_t const &dev_info,
                   l4_uint64_t &mmio_addr, l4_uint64_t &mmio_size, int &irq_num,
                   L4_irq_mode &irq_mode)
{
  for (unsigned i = 0;
       i < dev_info.num_resources && (!mmio_addr || !irq_num); ++i)
    {
      l4vbus_resource_t res;
      L4Re::chksys(dev.get_resource(i, &res));
      if (res.type == L4VBUS_RESOURCE_MEM)
        {
          if (!mmio_addr)
            {
              mmio_addr = res.start;
              mmio_size = res.end - res.start + 1;
            }
        }
      else if (res.type == L4VBUS_RESOURCE_IRQ)
        {
          if (!irq_num)
            {
              irq_num = res.start;
              irq_mode = L4_irq_mode(res.flags);
            }
        }
    }

  if (!mmio_addr)
    {
      info.printf("Device '%s' has no MMIO resource.\n", dev_info.name);
      return false;
    }

  if (!irq_num)
    {
      info.printf("Device '%s' has no IRQ resource.\n", dev_info.name);
      return false;
    }

  return true;
}

void
Factory::pci_dev(L4vbus::Pci_dev const &dev, l4_uint64_t &mmio_addr,
                 l4_uint64_t &mmio_size, int &irq_num, L4_irq_mode &irq_mode)
{
  l4_uint32_t addr;
  l4_uint32_t size;
  L4Re::chksys(dev.cfg_read(0x10, &addr, 32), "Read PCI cfg BAR0 (addr).");
  mmio_addr = addr & ~0xfU;
  L4Re::chksys(dev.cfg_write(0x10, ~0U, 32), "Write PCI cfg BAR0.");
  L4Re::chksys(dev.cfg_read(0x10, &size, 32), "Read PCI cfg BAR0 (size).");
  L4Re::chksys(dev.cfg_write(0x10, addr, 32), "Write PCI cfg BAR0 (restore.");
  if (size & 1)
    L4Re::throw_error(-L4_EINVAL, "First PCI BAR maps into memory");
  if ((size & 6) != 0)
    L4Re::throw_error(-L4_EINVAL, "First PCI BAR is 32-bits wide");
  mmio_size = -(size & ~0xfU);

  l4_uint32_t cmd;
  L4Re::chksys(dev.cfg_read(0x04, &cmd, 16), "Read PCI cfg command.");
  if (!(cmd & 4))
    {
      trace.printf("Enable PCI bus master.\n");
      cmd |= 4;
      L4Re::chksys(dev.cfg_write(0x04, cmd, 16), "Write PCI cfg command.");
    }

  unsigned char polarity;
  unsigned char trigger;
  irq_num = L4Re::chksys(dev.irq_enable(&trigger, &polarity),
                         "Enable interrupt.");

  if (trigger == 0)
    irq_mode = L4_IRQ_F_LEVEL_HIGH;
  else
    irq_mode = L4_IRQ_F_EDGE;
}

L4Re::Util::Shared_cap<L4Re::Dma_space>
Factory::create_dma_space(L4::Cap<L4vbus::Vbus> bus, long unsigned id)
{
  static std::map<long unsigned, L4Re::Util::Shared_cap<L4Re::Dma_space>> spaces;

  auto ires = spaces.find(id);
  if (ires != spaces.end())
    return ires->second;

  auto dma = L4Re::chkcap(L4Re::Util::make_shared_cap<L4Re::Dma_space>(),
                          "Allocate capability for DMA space.");
  L4Re::chksys(L4Re::Env::env()->user_factory()->create(dma.get()),
               "Create DMA space.");
  L4Re::chksys(
    bus->assign_dma_domain(id, L4VBUS_DMAD_BIND | L4VBUS_DMAD_L4RE_DMA_SPACE,
                           dma.get()),
    "Assignment of DMA domain.");
  spaces[id] = dma;
  return dma;
}

// Default implementation.
l4_uint32_t
Factory::guess_clock(l4_uint64_t)
{ return 0; }

cxx::Ref_ptr<Emmc::Base_device>
Factory::create_dev(L4vbus::Pci_dev const &dev, l4vbus_device_t const &dev_info,
                    L4::Cap<L4vbus::Vbus> bus, L4::Cap<L4::Icu> icu,
                    L4Re::Util::Object_registry *registry, unsigned max_seg,
                    Device_type_disable dt_disable)
{
  static unsigned device_nr = 0; // only for logging

  Factory *factory;
  l4_uint64_t mmio_addr = 0;
  l4_uint64_t mmio_size = 0;
  int irq_num = 0;
  L4_irq_mode irq_mode = L4_IRQ_F_LEVEL_HIGH;

  bool is_pcidev
    = l4vbus_subinterface_supported(dev_info.type, L4VBUS_INTERFACE_PCIDEV);
  if (is_pcidev)
    {
      l4_uint32_t vendor_device = 0;
      if (dev.cfg_read(0, &vendor_device, 32) != L4_EOK)
        return nullptr;

      l4_uint32_t class_code;
      L4Re::chksys(dev.cfg_read(8, &class_code, 32));
      class_code >>= 8;

      if (!(factory = Device_type_pci::find(class_code)))
        return nullptr;

      pci_dev(dev, mmio_addr, mmio_size, irq_num, irq_mode);
    }
  else
    {
      if (!(factory = Device_type_nopci::find(dev)))
        return nullptr;

      if (!nopci_dev(dev, dev_info, mmio_addr, mmio_size, irq_num, irq_mode))
        return nullptr;
    }

  unsigned long id = -1UL;
  for (auto i = 0u; i < dev_info.num_resources; ++i)
    {
      l4vbus_resource_t res;
      L4Re::chksys(dev.get_resource(i, &res), "Getting resource.");
      if (res.type == L4VBUS_RESOURCE_DMA_DOMAIN)
        {
          id = res.start;
          Dbg::trace().printf("Using device's DMA domain %lu.\n", res.start);
          break;
        }
    }

  if (id == -1UL)
    Dbg::trace().printf("Using VBUS global DMA domain.\n");

  info.printf("Device @ %08llx: %sinterrupt: %d, %s-triggered.\n",
              mmio_addr, is_pcidev ? "PCI " : "",
              irq_num, irq_mode == L4_IRQ_F_LEVEL_HIGH ? "level-high" : "edge");


  // XXX
  l4_uint32_t host_clock = 400000;
  if (l4_uint32_t guessed_clock = factory->guess_clock(mmio_addr))
    host_clock = guessed_clock;

  warn.printf("\033[33mAssuming host clock of %s.\033[m\n",
              Util::readable_freq(host_clock).c_str());

  try
    {
      auto iocap = dev.bus_cap();
      auto dma = create_dma_space(bus, id);

      return factory->create(device_nr++, mmio_addr, mmio_size, iocap, irq_num,
                             irq_mode, icu, dma, registry, host_clock, max_seg,
                             dt_disable);
    }
  catch (L4::Runtime_error const &e)
    {
      warn.printf("%s: %s. Skipping.\n", e.str(), e.extra_str());
      return nullptr;
    }
}

}
