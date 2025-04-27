/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Driver factory.
 */

#pragma once

#include <l4/cxx/hlist>
#include <l4/cxx/ref_ptr>
#include <l4/libblock-device/block_device_mgr.h>
#include <l4/vbus/vbus_pci>

#include "device.h"

namespace Emmc {

using Emmc_client_type = Block_device::Virtio_client<Emmc::Base_device>;

struct Device_factory
{
  using Device_type = Emmc::Base_device;
  using Client_type = Emmc_client_type;
  using Part_device = Emmc::Part_device;

  static cxx::unique_ptr<Client_type>
  create_client(cxx::Ref_ptr<Device_type> const &dev, unsigned numds,
                bool readonly)
  {
    return cxx::make_unique<Client_type>(dev, numds, readonly);
  }

  static cxx::Ref_ptr<Device_type>
  create_partition(cxx::Ref_ptr<Device_type> const &dev, unsigned partition_id,
                   Block_device::Partition_info const &pi)
  {
    return cxx::Ref_ptr<Device_type>(new Part_device(dev, partition_id, pi));
  }
};

using Base_device_mgr = Block_device::Device_mgr<Emmc::Base_device,
                                                 Device_factory>;

class Factory;

struct Device_type_nopci : public cxx::H_list_item_t<Device_type_nopci>
{
  char const *compatible;
  Factory *f;

  Device_type_nopci(char const *compatible, Factory *f)
  : compatible(compatible), f(f)
  {
    types.push_front(this);
  }

  static Factory *find(L4vbus::Pci_dev const &dev)
  {
    for (auto const *t : types)
      if (dev.is_compatible(t->compatible) == 1)
        return t->f;

    return nullptr;
  }

  static cxx::H_list_t<Device_type_nopci> types;
};

struct Device_type_pci : public cxx::H_list_item_t<Device_type_pci>
{
  l4_uint32_t class_code;
  Factory *f;

  Device_type_pci(l4_uint32_t class_code, Factory *f)
  : class_code(class_code), f(f)
  {
    types.push_front(this);
  }

  static Factory *find(l4_uint32_t class_code)
  {
    for (auto const *t : types)
      if (class_code == t->class_code)
        return t->f;

    return nullptr;
  }

  static cxx::H_list_t<Device_type_pci> types;
};

class Factory
{
public:
  virtual cxx::Ref_ptr<Emmc::Base_device>
    create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
           L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
           L4::Cap<L4::Icu> icu,
           L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
           L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
           unsigned max_seg, Device_type_disable dt_disable) = 0;

  virtual l4_uint32_t guess_clock(l4_uint64_t mmio_addr);

  virtual ~Factory() = 0;

  static cxx::Ref_ptr<Emmc::Base_device>
    create_dev(L4vbus::Pci_dev const &dev, l4vbus_device_t const &dev_info,
               L4::Cap<L4vbus::Vbus> bus, L4::Cap<L4::Icu> icu,
               L4Re::Util::Object_registry *registry,
               unsigned max_seg, Device_type_disable device_type_disable);

private:
  static bool nopci_dev(L4vbus::Device const &dev,
                        l4vbus_device_t const &dev_info,
                        l4_uint64_t &mmio_addr, l4_uint64_t &mmio_size,
                        int &irq_num, L4_irq_mode &irq_mode);
  static void pci_dev(L4vbus::Pci_dev const &dev,
                      l4_uint64_t &mmio_addr, l4_uint64_t &mmio_size,
                      int &irq_num, L4_irq_mode &irq_mode);
  static L4Re::Util::Shared_cap<L4Re::Dma_space>
    create_dma_space(L4::Cap<L4vbus::Vbus> bus, long unsigned id);
};

inline Factory::~Factory() {}

} // namespace Emmc
