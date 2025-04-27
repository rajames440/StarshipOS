/*
 * Copyright (C) 2014-2015, 2018-2022, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/drivers/hw_mmio_register_block>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/shared_cap>
#include <l4/re/util/object_registry>
#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>

#include <array>
#include <vector>
#include <stdio.h>
#include <cassert>

#include "ahci_port.h"
#include "ahci_types.h"

namespace Ahci {

/**
 * Encapsulates one single AHCI host bridge adapter.
 *
 * Includes a server loop for handling device interrupts.
 */
class Hba : public L4::Irqep_t<Hba>
{
private:
  /**
   * Self-attaching IO memory.
   */
  struct Iomem
  {
    L4Re::Rm::Unique_region<l4_addr_t> vaddr;

    Iomem(l4_addr_t phys_addr, L4::Cap<L4Re::Dataspace> iocap)
    {
      L4Re::chksys(L4Re::Env::env()->rm()->attach(&vaddr, L4_PAGESIZE,
                                                  L4Re::Rm::F::Search_addr
                                                  | L4Re::Rm::F::Cache_uncached
                                                  | L4Re::Rm::F::RW,
                                                  L4::Ipc::make_cap_rw(iocap),
                                                  phys_addr, L4_PAGESHIFT));
    }

    l4_addr_t port_base_address(unsigned num) const
    {
      return vaddr.get() + Port_base + Port_size * num;
    }

    enum Mem_config
    {
      Port_base = 0x100,
      Port_size = 0x80,
    };
 };

public:
  /**
   * Create a new AHCI HBA from a vbus PCI device.
   */
  Hba(L4vbus::Pci_dev const &dev,
      L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma);

  Hba(Hba const &) = delete;
  Hba(Hba &&) = delete;

  /**
   * Return the capability register of the HBA.
   */
  Hba_features features() const { return Hba_features(_regs[Regs::Hba::Cap]); }

  /**
   * Return a pointer to the given port
   *
   * Note that a port object is always returned, even when no
   * device is attached. It is the responsibility of the caller
   * to check for the state of the port.
   *
   * \param portno Port number.
   */
  Ahci_port *port(int portno) { return &_ports[portno]; }


  /**
   * Dispatch interrupts for the HBA to the ports.
   */
  void handle_irq();

  /**
   * Register the interrupt handler with a registry.
   *
   * \param icu      ICU to request the capability for the hardware interrupt.
   * \param registry Registry that dispatches the interrupt IPCs.
   *
   * \throws L4::Runtime_error Resources are not available or accessible.
   */
  void register_interrupt_handler(L4::Cap<L4::Icu> icu,
                                  L4Re::Util::Object_registry *registry);


  /**
   * Check ports for devices and initialize the ones that are found.
   *
   * \param callback Function called for each port that was successfully
   *                 initialized.
   */
  void scan_ports(std::function<void(Ahci_port *)> callback);

  int num_ports() { return _ports.size(); }

  /**
   * Test if a VBUS device is a AHCI HBA.
   *
   * \param dev      VBUS device to test.
   * \param dev_info Device information as returned by next_device()
   */
  static bool is_ahci_hba(L4vbus::Device const &dev,
                          l4vbus_device_t const &dev_info);

  /**
   * Check that address width of CPU and device are compatible.
   *
   * At the moment the HBA cannot specifically request memory below 4GB
   * from the dataspace manager. Therefore, it refuses to drive devices
   * on 64bit systems that are only capable of 32-bit addressing.
   * In practice, most systems will have their physical memory below
   * 4GB anyway, so this flag may be used to explicitly skip this check.
   */
  static bool check_address_width;
private:
  l4_uint32_t cfg_read(l4_uint32_t reg) const
  {
    l4_uint32_t val;
    L4Re::chksys(_dev.cfg_read(reg, &val, 32));

    return val;
  }

  l4_uint16_t cfg_read_16(l4_uint32_t reg) const
  {
    l4_uint32_t val;
    L4Re::chksys(_dev.cfg_read(reg, &val, 16));

    return val;
  }

  void cfg_write_16(l4_uint32_t reg, l4_uint16_t val)
  {
    L4Re::chksys(_dev.cfg_write(reg, val, 16));
  }

  L4vbus::Pci_dev _dev;
  Iomem _iomem;
  L4drivers::Register_block<32> _regs;
  unsigned char _irq_trigger_type;
  std::array<Ahci_port, 32> _ports;
};

}
