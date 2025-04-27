/*
 * Copyright (C) 2018-2020, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <string>

#include "ahci_port.h"

#include <l4/libblock-device/device.h>

namespace Ahci {

struct Device : Block_device::Device
{
  /// Return the maximum number of requests the device can handle in parallel.
  virtual unsigned max_in_flight() const = 0;
};

class Ahci_device : public Block_device::Device_with_notification_domain<Device>
{
  /**
   * Layout of device info page returned by the identify device command.
   *
   * Use for translation of the info page into the Device_info structure.
   */
  enum Identify_Device_Data
  {
    IID_serialnum_ofs           = 10,
    IID_serialnum_len           = 20,
    IID_firmwarerev_ofs         = 23,
    IID_firmwarerev_len         = 8,
    IID_modelnum_ofs            = 27,
    IID_modelnum_len            = 40,
    IID_capabilities            = 49,
    IID_addressable_sectors     = 60,
    IID_ata_major_rev           = 80,
    IID_ata_minor_rev           = 81,
    IID_enabled_features        = 85,
    IID_lba_addressable_sectors = 100,
    IID_logsector_size          = 117,
  };

  /**
   * Structure with general information about the device.
   *
   * \note This is just an internal struct that collects information
   *       about the hardware configuration relevant for the driver.
   */
  struct Device_info
  {
    /** Hardware ID string.
     *
     *  For real devices the serial number, for partitions their UUID. */
    std::string hid;
    /** Serial number as reported by hardware device */
    char serial_number[IID_serialnum_len + 1];
    /** Model number as reported by hardware device */
    char model_number[IID_modelnum_len + 1];
    /** Firmware revision as reported by hardware device */
    char firmware_rev[IID_firmwarerev_len + 1];
    /** Bitfield of supported ATA major revisions. */
    l4_uint16_t ata_major_rev;
    /** ATA version implemented by the device */
    l4_uint16_t ata_minor_rev;
    /** Size of a logical sector in bytes */
    l4_size_t sector_size;
    /** Number of logical sectors */
    l4_uint64_t num_sectors;
    /** Feature bitvector */
    struct
    {
      unsigned lba : 1;      ///< Logical block addressing supported
      unsigned dma : 1;      ///< DMA supported
      unsigned lba48 : 1;    ///< extended 48-bit addressing enabled
      unsigned s64a : 1;     ///< Bus supports 64bit addressing
      unsigned ro : 1;       ///< device is read=only (XXX not implemented)
    } features;

    /**
     * Fill the structure with information from the device identification page.
     *
     * \param info  Pointer to device info structure supplied by AHCI controller.
     */
    void set_device_info(l4_uint16_t const *info);
  };


public:
  Ahci_device(Ahci_port *port) : _port(port) {}

  bool is_read_only() const override
  { return _devinfo.features.ro; }

  bool match_hid(cxx::String const &hid) const override
  { return hid == cxx::String(_devinfo.hid.c_str(), _devinfo.hid.length()); }

  l4_uint64_t capacity() const override
  { return _devinfo.num_sectors * _devinfo.sector_size; }

  l4_size_t sector_size() const override
  { return _devinfo.sector_size; }

  l4_size_t max_size() const override
  { return 0x400000; }

  unsigned max_segments() const override
  { return Command_table::Max_entries; }

  unsigned max_in_flight() const override
  { return _port->max_slots(); }

  void reset() override
  {} // TODO

  int dma_map(Block_device::Mem_region *region, l4_addr_t offset,
              l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys) override
  {
    return _port->dma_map(region->ds(), offset,
                          num_sectors * _devinfo.sector_size, dir, phys);
  }

  int dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                L4Re::Dma_space::Direction dir) override
  {
    return _port->dma_unmap(phys, num_sectors * _devinfo.sector_size, dir);
  }

  int inout_data(l4_uint64_t sector,
                 Block_device::Inout_block const &blocks,
                 Block_device::Inout_callback const &cb,
                 L4Re::Dma_space::Direction dir) override;

  int flush(Block_device::Inout_callback const &cb) override;

  void start_device_scan(Block_device::Errand::Callback const &callback) override;

  static bool is_compatible_device(Ahci_port *port)
  { return port->device_type() == Ahci_port::Ahcidev_ata; }

private:
  Device_info _devinfo;
  Ahci_port *_port;
};



} // name space
