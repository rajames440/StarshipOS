/*
 * Copyright (C) 2020-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Device driver instance.
 *
 * So far, the driver is a template parameter for Device. This is convenient for
 * developing and for performance but eventually this causes headaches so this
 * might change in the future.
 */

#pragma once

#include <string>
#include <map>
#include <thread-l4>

#include <l4/cxx/string>
#include <l4/libblock-device/device.h>
#include <l4/libblock-device/part_device.h>

#include "debug.h"
#include "drv_sdhci.h"
#include "drv_sdhi.h"
#include "iomem.h"
#include "inout_buffer.h"

namespace Errand = Block_device::Errand;

namespace Emmc {

template <class Driver>
struct Dma_info;

struct Device_type_disable
{
  Mmc::Reg_ecsd::Ec196_device_type mmc{0};
  unsigned sd{0};
};

class Base_device
: public Block_device::Device,
  public Block_device::Device_discard_feature
{
public:
  void set_dma_map_all(bool enable)
  { _dma_map_all = enable; }

  bool _dma_map_all = false;
};

class Base_parent_device: public Base_device
{
public:
  virtual int dma_map_all(Block_device::Mem_region *, l4_addr_t, l4_size_t,
                          L4Re::Dma_space::Direction,
                          L4Re::Dma_space::Dma_addr *) = 0;

  virtual int dma_map_single(Block_device::Mem_region *, l4_addr_t, l4_size_t,
                             L4Re::Dma_space::Direction,
                             L4Re::Dma_space::Dma_addr *) = 0;

  virtual int dma_unmap_all(L4Re::Dma_space::Dma_addr, l4_size_t,
                            L4Re::Dma_space::Direction) = 0;

  virtual int dma_unmap_single(L4Re::Dma_space::Dma_addr, l4_size_t,
                               L4Re::Dma_space::Direction) = 0;
};

using Base_part_device = Block_device::Partitioned_device<Emmc::Base_device>;

class Part_device : public Base_part_device
{
public:
  using Base_part_device::Base_part_device;

private:
  int dma_map(Block_device::Mem_region *region, l4_addr_t offset,
              l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys) override
  {
    if (_dma_map_all)
      return static_cast<Base_parent_device *>(parent())->dma_map_all(
        region, offset, num_sectors, dir, phys);
    else
      return static_cast<Base_parent_device *>(parent())->dma_map_single(
        region, offset, num_sectors, dir, phys);
  }

  int dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                L4Re::Dma_space::Direction dir) override
  {
    if (_dma_map_all)
      return static_cast<Base_parent_device *>(parent())->dma_unmap_all(
        phys, num_sectors, dir);
    else
      return static_cast<Base_parent_device *>(parent())->dma_unmap_single(
        phys, num_sectors, dir);
  }
};

template <class Driver>
class Device
: public Block_device::Device_with_notification_domain<Base_parent_device>,
  public L4::Irqep_t<Device<Driver>>
{
public:
  enum
  {
    Dma_map_workaround = true,  ///< See #CD-202!
    Sector_size = 512U,
    Hid_max_length = 36,
    Voltage_delay_ms = 10,      ///< Delay after changing voltage [us]
    Stats_delay_us = 1000000,   ///< Delay between showing stats (info+) [us]
    Timeout_irq_us = 100000,    ///< timeout for receiving IRQs [us]
    Max_size = 4 << 20,
  };

  enum Medium_type
  {
    T_unknown,
    T_sd,
    T_mmc,
  };

  Device(int nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap,
         L4::Cap<L4Re::Mmio_space> mmio_space,
         int irq_num, L4_irq_mode irq_mode, L4::Cap<L4::Icu> icu,
         L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry,
         l4_uint32_t host_clock, int max_seg,
         Device_type_disable dt_disable);

  void handle_irq();

  void dma_unmap_region(Dma_info<Driver> *dma_info);

private:
  bool is_read_only() const override
  { return false; }

  bool match_hid(cxx::String const &hid) const override
  { return hid == cxx::String(_hid); }

  l4_uint64_t capacity() const override
  { return _num_sectors * Sector_size; }

  l4_size_t sector_size() const override
  { return Sector_size; }

  /**
   * Maximum size of one segment.
   *
   * Actually it should be possible to handle request with a size up to
   * 65535 * 512 = 32MB - 512.
   */
  l4_size_t max_size() const override
  {
    return _drv.provided_bounce_buffer()
      ? cxx::min(_drv._bb_size / _max_seg, l4_size_t{Max_size})
      : l4_size_t{Max_size};
  }

  /**
   * Without bounce buffer it should be possible to handle more than 1
   * segment.
   */
  unsigned max_segments() const override
  { return _max_seg; }

  Discard_info discard_info() const override
  {
    Discard_info di;

    // discard() currently returns -L4_EINVAL
    di.max_discard_sectors = 0;
    di.max_discard_seg = 0;
    di.discard_sector_alignment = 0;

    // discard() currently returns -L4_EINVAL
    di.max_write_zeroes_sectors = 0;
    di.max_write_zeroes_seg = 0;

    return di;
  }

  void receive_irq(bool is_data) const;

  void init_done();

  void reset() override;

  int dma_map_all(Block_device::Mem_region *region, l4_addr_t offset,
              l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys) override;

  int dma_map_single(Block_device::Mem_region *region, l4_addr_t offset,
              l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys) override;

  int dma_map(Block_device::Mem_region *region, l4_addr_t offset,
              l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys) override;

  int dma_unmap_all(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                L4Re::Dma_space::Direction dir) override;

  int dma_unmap_single(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                L4Re::Dma_space::Direction dir) override;

  int dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                L4Re::Dma_space::Direction dir) override;

  int inout_data(l4_uint64_t sector,
                 Block_device::Inout_block const &blocks,
                 Block_device::Inout_callback const &cb,
                 L4Re::Dma_space::Direction dir) override;

  int flush(Block_device::Inout_callback const &cb) override;

  int discard(l4_uint64_t offset, Block_device::Inout_block const &block,
              Block_device::Inout_callback const &cb, bool discard) override;

  void start_device_scan(Errand::Callback const &cb) override;

  void unmask_interrupt() const;

  enum Work_status
  {
    Work_done,
    More_work,
  };

  void cmd_queue_kick()
  {
    if (_drv.cmd_queue_kick())
      unmask_interrupt();
  }

  l4_uint64_t bytes_transferred(Cmd const *cmd) const
  { return (l4_uint64_t)cmd->sectors * sector_size(); }

  void handle_irq_inout(Cmd *cmd);
  Work_status handle_irq_inout_sdma(Cmd *cmd);
  Work_status transfer_block_sdma(Cmd *cmd);
  void set_block_count_adma2(Cmd *cmd);
  Work_status handle_irq_inout_adma2(Cmd *cmd);

  void cmd_exec(Cmd *cmd)
  { _drv.cmd_exec(cmd); }

  void reset_sdio(Cmd *cmd);
  bool power_up_sd(Cmd *cmd);
  bool power_up_mmc(Cmd *cmd);

  void mmc_set_timing(Cmd *cmd, Mmc::Reg_ecsd::Ec185_hs_timing::Timing timing,
                      Mmc::Timing mmc_timing, l4_uint32_t freq,
                      bool strobe = false);

  void mmc_set_bus_width(Cmd *cmd, Mmc::Reg_ecsd::Ec183_bus_width::Width width,
                         bool strobe = false);

  void adapt_ocr(Mmc::Reg_ocr ocr_dev, Mmc::Arg_acmd41_sd_send_op *a41);

  void exec_mmc_switch(Cmd *cmd, l4_uint8_t idx, l4_uint8_t val,
                       bool with_status = true);

  void mmc_app_cmd(Cmd *cmd, l4_uint32_t cmdval, l4_uint32_t arg,
                   l4_uint32_t datalen = 0, l4_uint64_t dataphys = 0,
                   l4_addr_t datavirt = 0);

  void show_statistics();
  void show_csd(Mmc::Reg_csd const &csd);
  static std::string readable_product(std::string const s);

  void claim_bounce_buffer(char const *cap_name);

  /// Device type (must be null-terminated)
  char _hid[Hid_max_length];

  Driver _drv;                  ///< driver instance
  int _irq_num;                 ///< interrupt number
  L4_irq_mode _irq_mode;        ///< IRQ mode
  bool _irq_unmask_at_icu;      ///< true: interrupt needs to be ack'd at ICU
  L4::Cap<L4::Irq> _irq;        ///< interrupt capability
  L4::Cap<L4::Icu> _icu;        ///< ICU capability
  L4Re::Util::Shared_cap<L4Re::Dma_space> _dma;
  int _max_seg;

  /// Device-related
  l4_uint64_t _addr_mult = 1;   ///< sector size multiplier
  l4_uint64_t _num_sectors = 0; ///< number of sectors of this device
  l4_uint16_t _rca = 0x0001;    ///< device address: MMC: assigned by the host
                                ///<                 SD:  assigned by the medium
  l4_uint32_t _mmc_rev = 0;     ///< eMMC revision
  l4_uint32_t _prg_cnt = 0;     ///< number of times to wait for prg state
  std::map<l4_uint8_t, l4_uint32_t> _prg_map; ///< prg state per SWITCH
  Medium_type _type = T_unknown; ///< medium type
  bool _has_cmd23 = true;       ///< device has auto CMD23 (default for eMMC)

  /// MMC (_type = T_mmc)
  Mmc::Reg_ecsd::Ec196_device_type _device_type_restricted;
  Mmc::Reg_ecsd::Ec196_device_type _device_type_selected;
  bool        _enh_strobe = false;
  l4_uint64_t _size_user = 0;   ///< size of the user partition in bytes
  l4_uint64_t _size_boot12 = 0; ///< size of the boot{1,2} partitions in bytes
  l4_uint64_t _size_rpmb = 0;   ///< size of the RPMB partition in bytes

  /// SD (_type = T_sd)
  Mmc::Timing _sd_timing;

  /// Device initialization
  std::thread _init_thread;
  L4Re::Util::Object_registry *_registry;

  /// EXT_CSD register content (currently also used for other registers)
  Inout_buffer _io_buf;
  Mmc::Reg_ecsd const &_ecsd;

  /// Bounce buffer.
  L4Re::Rm::Unique_region<l4_addr_t> _bb_region;

  /// Statistics
  l4_cpu_time_t _init_time = 0;
  l4_cpu_time_t _stat_time = 0;
  l4_uint64_t   _stat_ints = 0;

  Dbg warn;
  Dbg info;
  Dbg trace;
  Dbg trace2;

  /// Mask for bits in device_type which should be ignored.
  Device_type_disable _device_type_disable;

  constexpr char const *yes_no(unsigned bit) { return bit ? "yes" : "no"; }
  constexpr char const *yes_na(unsigned bit) { return bit ? "yes" : "N/A"; }

  // ::::: See #CD-202 ::::::::
  //
  // Dataspace + offset => phys
  struct Phys_entry
  {
    L4Re::Dma_space::Dma_addr phys;
    l4_size_t sectors;
    l4_uint32_t refcnt;
  };
  // Phys => Dataspace + offset
  struct Ds_offs_entry
  {
    l4_cap_idx_t ds;
    l4_addr_t offset;
  };
  typedef std::map<l4_addr_t, Phys_entry> Offs_entry;
  std::map<l4_cap_idx_t, Offs_entry> _ds_offs_map;
  std::map<L4Re::Dma_space::Dma_addr, Ds_offs_entry> _phys_map;
  //
  // ::::::::::::::::::::::::::
};

template <class Driver>
struct Dma_info : public Block_device::Dma_region_info
{
  L4Re::Dma_space::Dma_addr addr;
  l4_size_t size;
  cxx::Ref_ptr<Block_device::Device> device;

  Dma_info() = delete;
  explicit Dma_info(L4Re::Dma_space::Dma_addr addr, l4_size_t size,
                    cxx::Ref_ptr<Block_device::Device> device)
  : addr(addr), size(size), device(device)
  {}

  virtual ~Dma_info() override
  {
    static_cast<Emmc::Device<Driver> *>(device.get())->dma_unmap_region(this);
  }
};

} // namespace Emmc

#include "device-impl.h"
