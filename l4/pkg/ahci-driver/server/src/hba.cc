/*
 * Copyright (C) 2014-2015, 2017-2022, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/re/env>
#include <l4/re/dataspace>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>
#include <l4/vbus/vbus_interfaces.h>
#include <cstring>
#include <endian.h>

#include "hba.h"
#include "debug.h"

#if (__BYTE_ORDER == __BIG_ENDIAN)
# error "Big endian byte order not implemented."
#endif

static Dbg trace(Dbg::Trace, "hba");

namespace Ahci {

bool Hba::check_address_width = true;

Hba::Hba(L4vbus::Pci_dev const &dev,
         L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma)
: _dev(dev),
  _iomem(cfg_read(0x24) & 0xFFFFF000,
         L4::cap_reinterpret_cast<L4Re::Dataspace>(_dev.bus_cap())),
  _regs(new L4drivers::Mmio_register_block<32>(_iomem.vaddr.get()))
{
  trace.printf("Device registers  0x%x @ 0%lx, caps: 0x%x  caps2: 0x%x\n",
               cfg_read(0x24) & 0xFFFFF000, _iomem.vaddr.get(),
               _regs[Regs::Hba::Cap].read(), _regs[Regs::Hba::Cap2].read());

  l4_uint16_t cmd = cfg_read_16(0x04);
  if (!(cmd & 4))
    {
      trace.printf("Enabling PCI bus master\n");
      cfg_write_16(0x04, cmd | 4);
    }

  // set AHCI mode -- XXX done by BIOS?
  _regs[Regs::Hba::Ghc].set(Regs::Hba::Ghc_ae);

  // setup ports
  Hba_features feats = features();

  if (check_address_width && sizeof(l4_addr_t) == 8 && !feats.s64a())
    L4Re::chksys(-L4_ENOSYS,
                 "Cannot address 32bit devices on 64bit system. "
                 "Start driver with -A to disable test.");

  l4_uint32_t ports = _regs[Regs::Hba::Pi];
  trace.printf("Port information: 0x%x\n", ports);

  int portno = 0;
  unsigned buswidth = feats.s64a() ? 64 : 32;
  for (auto &p : _ports)
    {
      if (ports & (1 << portno))
        {
          int ret = p.attach(_iomem.port_base_address(portno), buswidth, dma);
          trace.printf("Registration of port %d %s(%i) @0x%lx\n",
                       portno,
                       ret < 0 ? "failed" : "done", ret,
                       _iomem.port_base_address(portno));
        }
      else
        trace.printf("Port %d is disabled @0x%lx\n",
                       portno, _iomem.port_base_address(portno));
      ++portno;
    }
}

void Hba::scan_ports(std::function<void(Ahci_port *)> callback)
{
  // the raw value is 0-based, thus add one to get the real number
  int ncs = features().ncs() + 1;
  for (auto &p : _ports)
    {
      if (p.device_type() != Ahci_port::Ahcidev_none)
        {
          auto port = &p;
          p.initialize(
            [=]()
              {
                try
                  {
                    port->initialize_memory(ncs);
                    port->enable(
                      [=]()
                       {
                         if (port->is_ready())
                           callback(port);
                         else
                           callback(nullptr);
                       });
                  }
                catch (L4::Runtime_error const &e)
                  {
                    Err().printf("Could not enable port: %s\n", e.str());
                    callback(nullptr);
                  }
              });
        }
      else
        callback(nullptr);
    }
}


void
Hba::handle_irq()
{
  l4_uint32_t is = _regs[Regs::Hba::Is];
  l4_uint32_t is_clear = is;

  for (auto &p : _ports)
    {
      if (is & 1)
        p.process_interrupts();
      is >>= 1;
    }

  if (!_irq_trigger_type)
    obj_cap()->unmask();

  // clear all status bits
  _regs[Regs::Hba::Is] = is_clear;
}


void
Hba::register_interrupt_handler(L4::Cap<L4::Icu> icu,
                                L4Re::Util::Object_registry *registry)
{
  // find the interrupt
  unsigned char polarity;
  int irq = L4Re::chksys(_dev.irq_enable(&_irq_trigger_type, &polarity),
                         "Enabling interrupt.");

  Dbg::info().printf("Device: interrupt : %d trigger: %d, polarity: %d\n",
                     irq, (int)_irq_trigger_type, (int)polarity);
  trace.printf("Device: interrupt status: 0x%x\n", _regs[Regs::Hba::Is].read());

  _regs[Regs::Hba::Ghc].clear(Regs::Hba::Ghc_ie);

  trace.printf("Registering server with registry....\n");
  auto cap = L4Re::chkcap(registry->register_irq_obj(this),
                          "Registering IRQ server object.");

  trace.printf("Binding interrupt %d...\n", irq);
  L4Re::chksys(l4_error(icu->bind(irq, cap)), "Binding interrupt to ICU.");

  trace.printf("Unmasking interrupt...\n");
  L4Re::chksys(l4_ipc_error(cap->unmask(), l4_utcb()),
               "Unmasking interrupt");

  trace.printf("Enabling HBA interrupt...\n");
  _regs[Regs::Hba::Is].write(0xFFFFFFFF);
  _regs[Regs::Hba::Ghc].set(Regs::Hba::Ghc_ie);

  trace.printf("Attached to interrupt %d\n", irq);
}


bool
Hba::is_ahci_hba(L4vbus::Device const &dev, l4vbus_device_t const &dev_info)
{
  if (!l4vbus_subinterface_supported(dev_info.type, L4VBUS_INTERFACE_PCIDEV))
    return false;

  L4vbus::Pci_dev const &pdev = static_cast<L4vbus::Pci_dev const &>(dev);
  l4_uint32_t val = 0;
  if (pdev.cfg_read(0, &val, 32) != L4_EOK)
    return false;

  // seems to be a PCI device
  trace.printf("Found PCI Device. Vendor 0x%x\n", val);
  L4Re::chksys(pdev.cfg_read(8, &val, 32));

  l4_uint32_t class_code = val >> 8;

  // XXX according to spec 01:04:00 works for RAID capable AHCI hosts
  // but dunno how to check that it is AHCI indeed
  return (class_code == 0x10601);
}

}
