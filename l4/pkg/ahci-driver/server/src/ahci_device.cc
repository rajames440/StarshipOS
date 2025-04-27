/*
 * Copyright (C) 2015, 2017-2020, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <algorithm>

#include "ahci_device.h"
#include "ahci_types.h"

#include <l4/libblock-device/inout_memory.h>

namespace {

  /**
   * Helper function to convert AHCI ID strings.
   *
   * param      id   Pointer to the start of the device info structure.
   * param[out] s    Pointer to where the resulting string should be stored.
   * param      ofs  Word (2-byte) offset within the device info structure from
   *                 where the ID string should be retrieved.
   * param      len  The length of the ID string in bytes.
   */
  void
  id2str(l4_uint16_t const *id, char *s, unsigned int ofs, unsigned int len)
  {
    unsigned int c;

    // we need to advance 2 bytes at a time because we handle two bytes per step
    for (; len > 0; len = len - 2, ++ofs)
      {
        c = id[ofs] >> 8;
        s[0] = c;
        c = id[ofs] & 0xff;
        s[1] = c;
        s += 2;
      }
    s[0] = 0;
  }

} // name space

namespace Ata { namespace Cmd {

// only contains commands used in this file
enum Ata_commands
{
  Id_device         = 0xec,
  Id_packet_device  = 0xa1,
  Read_dma          = 0xc8,
  Read_dma_ext      = 0x25,
  Read_sector       = 0x20,
  Read_sector_ext   = 0x24,
  Write_dma         = 0xca,
  Write_dma_ext     = 0x35,
  Write_sector      = 0x30,
  Write_sector_ext  = 0x34,
};

} } // name space

namespace Errand = Block_device::Errand;

void
Ahci::Ahci_device::start_device_scan(Errand::Callback const &callback)
{
  // temporarily assume 512-byte sectors for reading the info page
  _devinfo.sector_size = 512;
  auto infopage
    = cxx::make_ref_obj<Block_device::Inout_memory<Ahci_device>>(
        1, this, L4Re::Dma_space::Direction::From_device);


  Dbg::trace().printf("Reading device info...(infopage at %p)\n",
                      infopage->get<void>(0));

  auto cb = [=] (int error, l4_size_t)
              {
                printf("Infopage read from device.\n");
                infopage->unmap();
                if (error == L4_EOK)
                  {
                    _devinfo.features.s64a = _port->bus_width() == 64;
                    _devinfo.set_device_info(infopage->get<l4_uint16_t>(0));

                    Dbg info(Dbg::Info);
                    info.printf("Serial number: <%s>\n", _devinfo.serial_number);
                    info.printf("Model number: <%s>\n", _devinfo.model_number);
                    info.printf("LBA: %s  DMA: %s\n",
                                _devinfo.features.lba ? "yes": "no",
                                _devinfo.features.dma ? "yes": "no");
                    info.printf("Number of sectors: %llu sector size: %zu\n",
                                _devinfo.num_sectors, _devinfo.sector_size);
                  }
                callback();
              };

  // XXX should go in some kind of queue, if busy, instead of polling
  Errand::poll(10, 10000,
               [=] ()
                 {
                   Fis::Taskfile task;
                   Fis::Datablock data = infopage->inout_block();
                   task.command = Ata::Cmd::Id_device;
                   task.sector_size = 512;
                   task.flags = 0;
                   task.icc = 0;
                   task.control = 0;
                   task.device = 0;
                   task.data = &data;
                   int ret = _port->send_command(task, cb);
                   if (ret < 0 && ret != -L4_EBUSY) {
                     callback();
                     }
                   return ret != -L4_EBUSY;
                 },
               [=] (bool ret)
                 {
                   if (!ret)
                     callback();
                 }
              );
}

int
Ahci::Ahci_device::inout_data(l4_uint64_t sector,
                              Block_device::Inout_block const &blocks,
                              Block_device::Inout_callback const &cb,
                              L4Re::Dma_space::Direction dir)
{
  l4_size_t numsec = 0;
  for (auto const *block = &blocks; block; block = block->next.get())
    numsec += block->num_sectors;

  if (_devinfo.features.lba48)
    {
      if (numsec <= 0 || numsec > 65536 || sector > ((l4_uint64_t)1 << 48))
        {
          Err().printf("Client error: sector number out of range.\n");
          return -L4_EINVAL;
        }

      if (numsec == 65536)
        numsec = 0;
    }
  else
    {
      if (numsec <= 0 || numsec > 256 || sector > (1 << 28))
        {
          Err().printf("Client error: invalid sector number\n");
          return -L4_EINVAL;
        }

      if (numsec == 256)
        numsec = 0;
    }

  // check that 32bit devices get only 32bit addresses
  if ((sizeof(l4_addr_t) == 8) && !_devinfo.features.s64a
      && (sector >= 0x100000000L))
    {
      Err().printf("Client error: 64bit address for 32bit device\n");
      return -L4_EINVAL;
    }

  Fis::Taskfile task;

  if (dir == L4Re::Dma_space::Direction::To_device)
    {
      task.flags = Fis::Chf_write;
      if (_devinfo.features.dma)
        task.command = _devinfo.features.lba48 ? Ata::Cmd::Write_dma_ext
                                               : Ata::Cmd::Write_dma;
      else
        task.command = _devinfo.features.lba48 ? Ata::Cmd::Write_sector_ext
                                               : Ata::Cmd::Write_sector;
    }
  else if (dir == L4Re::Dma_space::Direction::From_device)
    {
      task.flags = 0;
      if (_devinfo.features.dma)
        task.command = _devinfo.features.lba48 ? Ata::Cmd::Read_dma_ext
                                               : Ata::Cmd::Read_dma;
      else
        task.command = _devinfo.features.lba48 ? Ata::Cmd::Read_sector_ext
                                               : Ata::Cmd::Read_sector;
    }

  task.lba = sector;
  task.count = numsec;
  task.device = 0x40;
  task.data = &blocks;
  task.sector_size = _devinfo.sector_size;
  task.icc = 0;
  task.control = 0;

  int ret = _port->send_command(task, cb);
  Dbg::trace().printf("IO to disk starting sector 0x%llx via slot %d\n",
                      sector, ret);

  return (ret >= 0) ? L4_EOK : ret;
}

int
Ahci::Ahci_device::flush(Block_device::Inout_callback const &cb)
{
  // TODO: flush the internal caches before completing the FLUSH
  cb(0, 0);
  return L4_EOK;
}

void
Ahci::Ahci_device::Device_info::set_device_info(l4_uint16_t const *info)
{
  id2str(info, serial_number, IID_serialnum_ofs, IID_serialnum_len);
  id2str(info, firmware_rev, IID_firmwarerev_ofs, IID_firmwarerev_len);
  id2str(info, model_number, IID_modelnum_ofs, IID_modelnum_len);
  ata_major_rev = info[IID_ata_major_rev];
  // normalize unreported version to 0
  if (ata_major_rev == 0xFFFF)
    ata_major_rev = 0;
  ata_minor_rev = info[IID_ata_minor_rev];

  // create HID from serial number
  hid = std::string(serial_number);
  // trim whitespace from the beginning
  hid.erase(hid.begin(), std::find_if(hid.begin(), hid.end(),
                                      [](int c) { return !std::isspace(c); }));
  // trim whitespace from the end
  hid.erase(std::find_if(hid.rbegin(), hid.rend(),
                         [](int c) { return !std::isspace(c); }).base(),
            hid.end());

  features.lba = info[IID_capabilities] >> 9;
  features.dma = info[IID_capabilities] >> 8;
  features.lba48 = info[IID_enabled_features + 1] >> 10;
  // XXX where is the read-only bit hiding again?
  features.ro = 0;


  sector_size = 2 * (l4_size_t(info[IID_logsector_size + 1]) << 16
                     | l4_size_t(info[IID_logsector_size]));
  if (sector_size < 512)
    sector_size = 512;
  if (features.lba48)
    num_sectors = (l4_uint64_t(info[IID_lba_addressable_sectors + 2]) << 32)
                  | (l4_uint64_t(info[IID_lba_addressable_sectors + 1]) << 16)
                  | l4_uint64_t(info[IID_lba_addressable_sectors]);
  else
    num_sectors = (l4_uint64_t(info[IID_addressable_sectors + 1]) << 16)
                  | l4_uint64_t(info[IID_addressable_sectors]);
}
