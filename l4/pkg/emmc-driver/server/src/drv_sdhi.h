/*
 * Copyright (C) 2021, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Driver for SDHI controllers found on RCar3 platforms.
 */

#pragma once

#include "drv.h"

namespace Emmc {

class Sdhi : public Drv<Sdhi>
{
  friend Drv;

public:
  static bool dma_adma2() { return false; }
  static bool auto_cmd12() { return false; }
  static bool auto_cmd23() { return false; }
  static bool bounce_buffer_if_required() { return false; }

private:

public:
  explicit Sdhi(int nr,
                L4::Cap<L4Re::Dataspace> iocap,
                L4::Cap<L4Re::Mmio_space> mmio_space,
                l4_uint64_t mmio_base, l4_uint64_t mmio_size,
                L4Re::Util::Shared_cap<L4Re::Dma_space> const &,
                l4_uint32_t, Receive_irq receive_irq);

  /** Initialize controller registers. */
  void init();

  /** IRQ handler. */
  Cmd *handle_irq();

  /** Disable all controller interrupts. */
  void mask_interrupts();

  /** Show interrupt status word if 'trace' debug level is enabled. */
  void show_interrupt_status(char const *s) const
  { (void)s; }

  /** Set clock and timing. */
  void set_clock_and_timing(l4_uint32_t clock, Mmc::Timing mmc_timing,
                            bool strobe = false);

  /** Set bus width. */
  void set_bus_width(Mmc::Bus_width bus_width);

  /** Set voltage (3.3V or 1.8V). */
  void set_voltage(Mmc::Voltage mmc_voltage)
  { (void)mmc_voltage; }

  /** Return true if any of the UHS timings is supported by the controller. */
  bool supp_uhs_timings(Mmc::Timing timing) const
  { (void)timing; return false; }

  /** Return true if the selected timing needs tuning. */
  bool needs_tuning_sdr50() const
  { return false; }

  /** Return true if the power limit is supported by the controller. */
  bool supp_power_limit(Mmc::Power_limit power) const
  { (void)power; return false; }

  /** Return true if tuning has finished. */
  bool tuning_finished(bool *success)
  { (void)success; return false; }

  void reset_tuning() {}
  void enable_auto_tuning() {}

  /** Return true if the card is busy. */
  bool card_busy() const
  { return !Reg_sd_info(_regs).dat0(); }

  /** Return supported power values by the controller. */
  Mmc::Reg_ocr supported_voltage() const
  {
    Mmc::Reg_ocr ocr(0);
    ocr.mv3200_3300() = 1;
    ocr.mv3300_3400() = 1;
    return ocr;
  }

  bool xpc_supported(Mmc::Voltage) const
  { return true; }

  void dump() const;

private:
  enum Regs
  {
    Sd_cmd              = 0x0000,
    Sd_portsel          = 0x0008,
    Sd_arg              = 0x0010,
    Sd_arg1             = 0x0018,
    Sd_stop             = 0x0020,
    Sd_seccnt           = 0x0028,
    Sd_rsp10            = 0x0030,
    Sd_rsp1             = 0x0038,
    Sd_rsp32            = 0x0040,
    Sd_rsp3             = 0x0048,
    Sd_rsp54            = 0x0050,
    Sd_rsp5             = 0x0058,
    Sd_rsp76            = 0x0060,
    Sd_rsp7             = 0x0068,
    Sd_info1            = 0x0070,
    Sd_info2            = 0x0078,
    Sd_info1_mask       = 0x0080,
    Sd_info2_mask       = 0x0088,
    Sd_clk_ctrl         = 0x0090,
    Sd_size             = 0x0098,
    Sd_option           = 0x00a0,
    Sd_err_sts1         = 0x00b0,
    Sd_err_sts2         = 0x00b8,
    Sd_buf0             = 0x00c0,
    Sdio_mode           = 0x00d0,
    Sdio_info1          = 0x00d8,
    Sdio_info1_mask     = 0x00e0,
    Cc_ext_mode         = 0x0360,
    Soft_rst            = 0x0380,
    Version             = 0x0388,
    Host_mode           = 0x0390,
    Sdif_mode           = 0x0398,
    Dm_cm_info1         = 0x0840,
    Dm_cm_info1_mask    = 0x0848,
    Dm_cm_info2         = 0x0850,
    Dm_cm_info2_mask    = 0x0858,
    Dm_cm_seq_regset    = 0x0800,
    Dm_cm_seq_ctrl      = 0x0810,
    Dm_cm_dtran_mode    = 0x0820,
    Dm_cm_dtran_ctrl    = 0x0828,
    Dm_cm_rst           = 0x0830,
    Dm_cm_seq_stat      = 0x0868,
    Dm_dtran_addr       = 0x0880,
    Dm_seq_cmd          = 0x08a0,
    Dm_seq_arg          = 0x08a8,
    Dm_seq_size         = 0x08b0,
    Dm_seq_seccnt       = 0x08b8,
    Dm_seq_rsp          = 0x08c0,
    Dm_seq_rsp_chk      = 0x08c8,
    Dm_seq_addr         = 0x08d0,
  };

  /**
   * Convenience class for implicit register offset handling.
   * It implements a bit field which is either initialized with zero, with a
   * certain value or by reading the corresponding device register. Then
   * certain bits of this bit field are read and / or manipulated. Finally the
   * raw value can be written back to the corresponding device register. The
   * assignment to the device register is stored in the `offs` template
   * parameter.
   */
  template <enum Regs offs>
  struct Reg
  {
    explicit Reg() : raw(0) {}
    explicit Reg(Hw_regs const &regs) : raw(regs[offs]) {}
    explicit Reg(l4_uint32_t v) : raw(v) {}
    l4_uint32_t read(Hw_regs &regs) { raw = regs[offs]; return raw; }
    void write(Hw_regs &regs)
    {
      switch (offs)
        {
        case Sd_cmd:
        case Sd_stop:
        case Sd_seccnt:
        case Sd_size:
        case Sd_option:
        case Sdio_mode:
        case Cc_ext_mode:
        case Host_mode:
          Util::poll(10000, [regs] { return !Reg_sd_info(regs).cbsy(); },
                     "Writing register (cbsy)");
          break;
        case Sd_clk_ctrl:
          Util::poll(10000, [regs] { return !!Reg_sd_info(regs).sclkdiven(); },
                     "Writing register (sclkdiven)");
          break;
        default:
          ;
        }
      printf("\033[33mwrite %04x <= %08x\033[m\n", offs, raw);
      regs[offs] = raw;
    }
    l4_uint32_t raw;
  };

  template <enum Regs offs>
  struct Reg_2_16
  {
    explicit Reg_2_16() : raw(0) {}
    explicit Reg_2_16(Hw_regs const &regs)
    : raw(regs.r<16>(offs) | ((l4_uint32_t)(regs.r<16>(offs + 8)) << 16)) {}
    explicit Reg_2_16(l4_uint32_t v) : raw(v) {}
    l4_uint32_t read(Hw_regs &regs)
    {
      raw = regs[offs] | (regs[offs + 8] << 16);
      return raw;
    }
    void write(Hw_regs &regs)
    {
      regs.r<16>(offs)     = raw & 0xffff;
      regs.r<16>(offs + 8) = raw >> 16;
    }
    l4_uint32_t raw;
  };

  // 0x00
  struct Reg_sd_cmd : public Reg<Sd_cmd>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(15, 15, md7, raw);      ///< must be always 0
    CXX_BITFIELD_MEMBER(14, 14, md6, raw);      ///< 1: no auto CMD12
    CXX_BITFIELD_MEMBER(13, 13, md5, raw);      ///< 1: multi-block transfer
    CXX_BITFIELD_MEMBER(12, 12, md4, raw);      ///< 1: read transfer
    CXX_BITFIELD_MEMBER(11, 11, md3, raw);      ///< 1: command with data
    CXX_BITFIELD_MEMBER(8, 10, mode, raw);      ///< response type
    enum Mode
    {
      Resp_normal = 0,
      Resp_none = 3,
      Resp_r1 = 4,
      Resp_r1b = 5,
      Resp_r2 = 6,
      Resp_r3 = 7,
    };
    CXX_BITFIELD_MEMBER(7, 7, c1, raw);         ///< must be always 0
    CXX_BITFIELD_MEMBER(6, 6, c0, raw);         ///< 1: ACMD
    CXX_BITFIELD_MEMBER(0, 5, cf, raw);         ///< command index
  };

  // 0x08
  struct Reg_sd_portsel : public Reg<Sd_portsel>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(9, 9, np1, raw);        ///< 0
    CXX_BITFIELD_MEMBER(8, 8, np0, raw);        ///< 1: one SD card port
  };

  // 0x10
  struct Reg_sd_arg : public Reg<Sd_arg>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(31, 31, cf39, raw);
    CXX_BITFIELD_MEMBER(30, 30, cf38, raw);
    CXX_BITFIELD_MEMBER(29, 29, cf37, raw);
    CXX_BITFIELD_MEMBER(28, 28, cf36, raw);
    CXX_BITFIELD_MEMBER(27, 27, cf35, raw);
    CXX_BITFIELD_MEMBER(26, 26, cf34, raw);
    CXX_BITFIELD_MEMBER(25, 25, cf33, raw);
    CXX_BITFIELD_MEMBER(24, 24, cf32, raw);
    CXX_BITFIELD_MEMBER(23, 23, cf31, raw);
    CXX_BITFIELD_MEMBER(22, 22, cf30, raw);
    CXX_BITFIELD_MEMBER(21, 21, cf29, raw);
    CXX_BITFIELD_MEMBER(20, 20, cf28, raw);
    CXX_BITFIELD_MEMBER(19, 19, cf27, raw);
    CXX_BITFIELD_MEMBER(18, 18, cf26, raw);
    CXX_BITFIELD_MEMBER(17, 17, cf25, raw);
    CXX_BITFIELD_MEMBER(16, 16, cf24, raw);
    CXX_BITFIELD_MEMBER(15, 15, cf23, raw);
    CXX_BITFIELD_MEMBER(14, 14, cf22, raw);
    CXX_BITFIELD_MEMBER(13, 13, cf21, raw);
    CXX_BITFIELD_MEMBER(12, 12, cf20, raw);
    CXX_BITFIELD_MEMBER(11, 11, cf19, raw);
    CXX_BITFIELD_MEMBER(10, 10, cf18, raw);
    CXX_BITFIELD_MEMBER(9, 9, cf17, raw);
    CXX_BITFIELD_MEMBER(8, 8, cf16, raw);
    CXX_BITFIELD_MEMBER(7, 7, cf15, raw);
    CXX_BITFIELD_MEMBER(6, 6, cf14, raw);
    CXX_BITFIELD_MEMBER(5, 5, cf13, raw);
    CXX_BITFIELD_MEMBER(4, 4, cf12, raw);
    CXX_BITFIELD_MEMBER(3, 3, cf11, raw);
    CXX_BITFIELD_MEMBER(2, 2, cf10, raw);
    CXX_BITFIELD_MEMBER(1, 1, cf9, raw);
    CXX_BITFIELD_MEMBER(0, 0, cf8, raw);
    CXX_BITFIELD_MEMBER(0, 31, cf8_cf39, raw);
  };

  // 0x20
  struct Reg_sd_stop : public Reg<Sd_stop>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(17, 17, hpimode, raw);  // HPI mode enable
    CXX_BITFIELD_MEMBER(16, 16, hpicmd, raw);   // HPI command issue
    CXX_BITFIELD_MEMBER(8, 8, sec, raw);        // block count enable
    CXX_BITFIELD_MEMBER(0, 0, stp, raw);        // stop
  };

  // 0x28
  struct Reg_sd_seccnt : public Reg<Sd_seccnt>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 31, cnt, raw);       // number of transfer blocks
  };

  // 0x30
  struct Reg_sd_rsp10 : public Reg<Sd_rsp10>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 31, r8_r39, raw);
  };

  // 0x38
  struct Reg_sd_rsp1 : public Reg<Sd_rsp1>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 15, r24_r39, raw);
  };

  // 0x40
  struct Reg_sd_rsp32 : public Reg<Sd_rsp32>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 31, r40_r71, raw);
  };

  // 0x48
  struct Reg_sd_rsp3 : public Reg<Sd_rsp3>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 15, r56_r71, raw);
  };

  // 0x50
  struct Reg_sd_rsp54 : public Reg<Sd_rsp54>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 31, r72_r103, raw);
  };

  // 0x58
  struct Reg_sd_rsp5 : public Reg<Sd_rsp5>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 15, r88_r103, raw);
  };

  // 0x60
  struct Reg_sd_rsp76 : public Reg<Sd_rsp76>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 23, r104_r127, raw);
  };

  // 0x68
  struct Reg_sd_rsp7 : public Reg<Sd_rsp7>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 7, r120_r127, raw);
  };

  // 0x70
  struct Reg_sd_info : public Reg_2_16<Sd_info1>
  {
    // Setting/reading hpires not possible!
    using Reg_2_16::Reg_2_16;
    explicit Reg_sd_info() : Reg_2_16()
    { res27() = 1; }
    explicit Reg_sd_info(l4_uint32_t v) : Reg_sd_info()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(31, 31, ila, raw);      ///< illegal access error
    CXX_BITFIELD_MEMBER(30, 30, cbsy, raw);     ///< command type register busy
    CXX_BITFIELD_MEMBER(29, 29, sclkdiven, raw); ///< 0: SD bus busy
    CXX_BITFIELD_MEMBER(27, 27, res27, raw);    ///< must be always 1
    CXX_BITFIELD_MEMBER(25, 25, bwe, raw);      ///< SD_BUF write enable
    CXX_BITFIELD_MEMBER(24, 24, bre, raw);      ///< SD_BUF read enable
    CXX_BITFIELD_MEMBER(23, 23, dat0, raw);     ///< SDDAT0 status
    CXX_BITFIELD_MEMBER(22, 22, err6, raw);     ///< response timeout
    CXX_BITFIELD_MEMBER(21, 21, err5, raw);     ///< SD_BUF illegal read access
    CXX_BITFIELD_MEMBER(20, 20, err4, raw);     ///< SD_BUF illegal write access
    CXX_BITFIELD_MEMBER(19, 19, err3, raw);     ///< data timeout
    CXX_BITFIELD_MEMBER(18, 18, err2, raw);     ///< END error
    CXX_BITFIELD_MEMBER(17, 17, err1, raw);     ///< CRC error
    CXX_BITFIELD_MEMBER(16, 16, err0, raw);     ///< CMD error
    CXX_BITFIELD_MEMBER(10, 10, info10, raw);   ///< SDDAT3 state
    CXX_BITFIELD_MEMBER(9, 9, info9, raw);      ///< SDDAT3 card insertion
    CXX_BITFIELD_MEMBER(8, 8, info8, raw);      ///< SDDAT3 card removal
    CXX_BITFIELD_MEMBER(7, 7, info7, raw);      ///< write protect
    CXX_BITFIELD_MEMBER(5, 5, info5, raw);      ///< ISDCD state
    CXX_BITFIELD_MEMBER(4, 4, info4, raw);      ///< ISDCD card insertion
    CXX_BITFIELD_MEMBER(3, 3, info3, raw);      ///< ISDCD card removal
    CXX_BITFIELD_MEMBER(2, 2, info2, raw);      ///< access end
    CXX_BITFIELD_MEMBER(0, 0, info0, raw);      ///< response end

    void clear_ints()
    {
      raw = 0;
      res27() = 1;
    }

    bool error()
    {
      return err0() || err1() || err2() || err3() || /* err4() || err5() || */ err6()
          || ila();
    }
  };

  // 0x80
  struct Reg_sd_info_mask : public Reg_2_16<Sd_info1_mask>
  {
    // Setting/reading imask16 not possible!
    using Reg_2_16::Reg_2_16;
    explicit Reg_sd_info_mask() : Reg_2_16(~0U)
    {
      res1() = 1;
      res5_7() = 7;
      res10_15() = 0x3f;
      res23() = 1;
      res26_30() = 0x1f;
    }
    explicit Reg_sd_info_mask(l4_uint32_t v) : Reg_sd_info_mask()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(31, 31, imask, raw);    ///< illegal access error
    CXX_BITFIELD_MEMBER(26, 30, res26_30, raw); ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(25, 25, bmask1, raw);   ///< SD_BUF write enb int masked
    CXX_BITFIELD_MEMBER(24, 24, bmask0, raw);   ///< SD_BUF read enb int masked
    CXX_BITFIELD_MEMBER(23, 23, res23, raw);    ///< must be always 1
    CXX_BITFIELD_MEMBER(22, 22, emask6, raw);   ///< response timeout int masked
    CXX_BITFIELD_MEMBER(21, 21, emask5, raw);   ///< SD_BUF ill read int masked
    CXX_BITFIELD_MEMBER(20, 20, emask4, raw);   ///< SD_BUF ill write int masked
    CXX_BITFIELD_MEMBER(19, 19, emask3, raw);   ///< data timeout  int masked
    CXX_BITFIELD_MEMBER(18, 18, emask2, raw);   ///< END error int masked
    CXX_BITFIELD_MEMBER(17, 17, emask1, raw);   ///< CRC error int masked
    CXX_BITFIELD_MEMBER(16, 16, emask0, raw);   ///< CMD error int masked
    // We actually ignore bit 16 imask16 (HPIRES interrupt enabled)!
    CXX_BITFIELD_MEMBER(10, 15, res10_15, raw); ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(9, 9, imask9, raw);     ///< SDDAT3 card insertion
    CXX_BITFIELD_MEMBER(8, 8, imask8, raw);     ///< SDDAT3 card removal
    CXX_BITFIELD_MEMBER(5, 5, res5_7, raw);     ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(4, 4, imask4, raw);     ///< ISDCD card insertion
    CXX_BITFIELD_MEMBER(3, 3, imask3, raw);     ///< ISDCD card removal
    CXX_BITFIELD_MEMBER(2, 2, imask2, raw);     ///< access end
    CXX_BITFIELD_MEMBER(1, 1, res1, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(0, 0, imask0, raw);     ///< response end

    void enable_ints()
    {
      imask0() = 0;
      imask2() = 0;
      imask3() = 0;
      imask4() = 0;
      //bmask0() = 0;
      //bmask1() = 0;
      emask6() = 0;
    }

    void disable_ints()
    {
      raw = ~0U;
    }
  };

  // 0x90
  struct Reg_sd_clk_ctrl : public Reg<Sd_clk_ctrl>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(9, 9, sdclk_offen, raw);
    CXX_BITFIELD_MEMBER(8, 8, sclken, raw);     ///< SD clock output ctrl enable
    CXX_BITFIELD_MEMBER(7, 7, div7, raw);       ///< SD clock
    CXX_BITFIELD_MEMBER(6, 6, div6, raw);
    CXX_BITFIELD_MEMBER(5, 5, div5, raw);
    CXX_BITFIELD_MEMBER(4, 4, div4, raw);
    CXX_BITFIELD_MEMBER(3, 3, div3, raw);
    CXX_BITFIELD_MEMBER(2, 2, div2, raw);
    CXX_BITFIELD_MEMBER(1, 1, div1, raw);
    CXX_BITFIELD_MEMBER(0, 0, div0, raw);
    CXX_BITFIELD_MEMBER(0, 7, div, raw);
    l4_uint32_t divisor() const
    {
      if (div() == 0xff)
        return 1;
      else if (div() == 0)
        return 2;
      else
        return div() << 2;
    }
    void set_divisor(unsigned divisor)
    {
      switch (divisor)
        {
        case 1 << 9:
        case 1 << 8:
        case 1 << 7:
        case 1 << 6:
        case 1 << 5:
        case 1 << 4:
        case 1 << 3:
        case 1 << 2: div() = divisor >> 2; break;
        case 1 << 1: div() = 0; break;
        case 1 << 0: div() = 0xff; break;
        default: L4Re::throw_error(-L4_EINVAL, "invalid divisor");
        }
    }
  };

  // 0x98
  struct Reg_sd_size : public Reg<Sd_size>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 9, len, raw);        ///< transfer data size (1-512)
  };

  // 0xa0
  struct Reg_sd_option : public Reg<Sd_option>
  {
    using Reg::Reg;
    explicit Reg_sd_option() : Reg()
    { res14() = 1; }
    explicit Reg_sd_option(l4_uint32_t v) : Reg_sd_option()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(15, 15, width, raw);    ///< bus width 1
    CXX_BITFIELD_MEMBER(14, 14, res14, raw);
    CXX_BITFIELD_MEMBER(13, 13, width8, raw);   ///< bus width 2
    CXX_BITFIELD_MEMBER(9, 9, extop, raw);
    CXX_BITFIELD_MEMBER(8, 8, toutmask, raw);
    CXX_BITFIELD_MEMBER(4, 7, top, raw);
    CXX_BITFIELD_MEMBER(0, 3, ctop, raw);
    void set_bus_width(Mmc::Bus_width bus_width)
    {
      switch (bus_width)
        {
        case Mmc::Width_1bit:
          width() = 1;
          width8() = 0;
          break;
        case Mmc::Width_4bit:
          width() = 0;
          width8() = 0;
          break;
        case Mmc::Width_8bit:
          width() = 0;
          width8() = 1;
          break;
        }
    }
    char const *str_bus_width() const
    { return width() ? "1-bit" : width8() ? "8-bit" : "4-bit"; }
  };

  // 0xb0
  struct Reg_sd_err_sts1 : public Reg<Sd_err_sts1>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(11, 11, e11, raw);
    CXX_BITFIELD_MEMBER(10, 10, e10, raw);
    CXX_BITFIELD_MEMBER(9, 9, e9, raw);
    CXX_BITFIELD_MEMBER(8, 8, e8, raw);
    CXX_BITFIELD_MEMBER(7, 7, e7, raw);
    CXX_BITFIELD_MEMBER(6, 6, e6, raw);
    CXX_BITFIELD_MEMBER(5, 5, e5, raw);
    CXX_BITFIELD_MEMBER(4, 4, e4, raw);
    CXX_BITFIELD_MEMBER(3, 3, e3, raw);
    CXX_BITFIELD_MEMBER(2, 2, e2, raw);
    CXX_BITFIELD_MEMBER(1, 1, e1, raw);
    CXX_BITFIELD_MEMBER(0, 0, e0, raw);
  };

  // 0xb8
  struct Reg_sd_err_sts2 : public Reg<Sd_err_sts2>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(6, 6, e6, raw);
    CXX_BITFIELD_MEMBER(5, 5, e5, raw);
    CXX_BITFIELD_MEMBER(4, 4, e4, raw);
    CXX_BITFIELD_MEMBER(3, 3, e3, raw);
    CXX_BITFIELD_MEMBER(2, 2, e2, raw);
    CXX_BITFIELD_MEMBER(1, 1, e1, raw);
    CXX_BITFIELD_MEMBER(0, 0, e0, raw);
  };

  // 0xc0
  struct Reg_sd_buf0 : public Reg<Sd_buf0>
  {
    using Reg::Reg;
  };

  // 0xd0
  struct Reg_sdio_mode : public Reg<Sdio_mode>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(9, 9, c52pub, raw);     ///< SDIO none abort
    CXX_BITFIELD_MEMBER(8, 8, ioabt, raw);      ///< SDIO abort
    CXX_BITFIELD_MEMBER(2, 2, rwreq, raw);      ///< read wait request
    CXX_BITFIELD_MEMBER(0, 0, iomod, raw);      ///< 1: enable SDIO interrupts
  };

  // 0xd8
  struct Reg_sdio_info1 : public Reg<Sdio_info1>
  {
    using Reg::Reg;
    explicit Reg_sdio_info1() : Reg()
    {
      res1() = 1;
      res2() = 1;
    }
    explicit Reg_sdio_info1(l4_uint32_t v) : Reg_sdio_info1()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(15, 15, exwt, raw);
    CXX_BITFIELD_MEMBER(14, 14, expub52, raw);
    CXX_BITFIELD_MEMBER(2, 2, res2, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(1, 1, res1, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(0, 0, ioirq, raw);
  };

  // 0xe0
  struct Reg_sdio_info1_mask : public Reg<Sdio_info1_mask>
  {
    using Reg::Reg;
    explicit Reg_sdio_info1_mask() : Reg()
    {
      res1() = 1;
      res2() = 1;
    }
    explicit Reg_sdio_info1_mask(l4_uint32_t v) : Reg_sdio_info1_mask()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(15, 15, mexwt, raw);
    CXX_BITFIELD_MEMBER(14, 14, mexpub52, raw);
    CXX_BITFIELD_MEMBER(2, 2, res2, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(1, 1, res1, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(0, 0, iomsk, raw);

    void disable_ints()
    {
      iomsk() = 1;
      res1() = 1;
      res2() = 1;
      mexpub52() = 1;
      mexwt() = 1;
    }
  };

  // 0x360
  struct Reg_cc_ext_mode : public Reg<Cc_ext_mode>
  {
    using Reg::Reg;
    explicit Reg_cc_ext_mode() : Reg()
    {
      res4() = 1;
      res12() = 1;
    }
    explicit Reg_cc_ext_mode(l4_uint32_t v) : Reg_cc_ext_mode()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(12, 12, res12, raw);    ///< must be always 1
    CXX_BITFIELD_MEMBER(4, 4, res4, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(1, 1, dmasdrw, raw);    ///< 1: SD_BUF r/w DMA transfer
  };

  // 0x380
  struct Reg_soft_rst : public Reg<Soft_rst>
  {
    using Reg::Reg;
    explicit Reg_soft_rst() : Reg()
    {
      res1() = 1;
      res2() = 1;
    }
    explicit Reg_soft_rst(l4_uint32_t v) : Reg_soft_rst()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(2, 2, res2, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(1, 1, res1, raw);       ///< must be always 1
    CXX_BITFIELD_MEMBER(0, 0, sdrst, raw);      ///< software reset of SD IF
  };

  // 0x388
  struct Reg_version : public Reg<Version>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 15, version, raw);
    enum
    {
      Sdhi_ver_gen3_sd = 0xcc10,                ///< channels 0 and 1
      Sdhi_ver_gen3_sdmmc = 0xcd10,             ///< channels 2 and 3
    };
  };

  // 0x390
  struct Reg_host_mode : public Reg<Host_mode>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(8, 8, buswidth, raw);   ///< 0: 16-bit, 1: 32-bit SDBUF
    CXX_BITFIELD_MEMBER(1, 1, endian, raw);     ///< 1: SD_BUF0 data swap
    CXX_BITFIELD_MEMBER(0, 0, wmode, raw);      ///< 0: 64-bit, 1: 16/32-bit
    enum Bus_width
    {
      Width_64bit,
      Width_32bit,
      Width_16bit
    };
    void set_bus_width(Bus_width width)
    {
      switch (width)
        {
        case Width_64bit:
          wmode() = 0;
          buswidth() = 0;
          break;
        case Width_32bit:
          wmode() = 1;
          buswidth() = 1;
          break;
        case Width_16bit:
          wmode() = 1;
          buswidth() = 0;
          break;
        }
    }
  };

  // 0x398
  struct Reg_sdif_mode : public Reg<Sdif_mode>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(8, 8, nochkcr, raw);    ///< CRC check mask
    CXX_BITFIELD_MEMBER(0, 0, hs400, raw);      ///< HS400 mode select
  };

  // 0x800
  struct Reg_dm_cm_seq_regset : public Reg<Dm_cm_seq_regset>
  {
    using Reg::Reg;
  };

  // 0x810
  struct Reg_dm_cm_seq_ctrl : public Reg<Dm_cm_seq_ctrl>
  {
    using Reg::Reg;
  };

  // 0x820
  struct Reg_dm_cm_dtran_mode : public Reg<Dm_cm_dtran_mode>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(16, 17, ch_num, raw);   ///< channel selector
    enum
    {
      Ch_0_write = 0,
      Ch_1_read = 1,
    };
    CXX_BITFIELD_MEMBER(4, 5, bus_width, raw);  ///< bus width
    enum { Bus_64bits = 3 };
    CXX_BITFIELD_MEMBER(0, 0, addr_mode, raw);  ///< address mode selector
    enum
    {
      Fixed = 0,
      Incr_addr = 1,
    };
  };

  // 0x828
  struct Reg_dm_cm_dtran_ctrl : public Reg<Dm_cm_dtran_ctrl>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 0, dm_start, raw);   ///< DMAC start
  };

  // 0x830
  struct Reg_dm_cm_rst : public Reg<Dm_cm_rst>
  {
    using Reg::Reg;
    explicit Reg_dm_cm_rst() : Reg()
    {
      res1_7() = 0x7f;
      res10_31() = 0x3fffff;
    }
    explicit Reg_dm_cm_rst(l4_uint32_t v) : Reg_dm_cm_rst()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(10, 31, res10_31, raw); ///< all bits should be 1
    CXX_BITFIELD_MEMBER(8, 9, dtranrst, raw);   ///< soft reset each channel
    CXX_BITFIELD_MEMBER(1, 7, res1_7, raw);     ///< all bits should be a1
    CXX_BITFIELD_MEMBER(0, 0, seqrst, raw);     ///< sequencer soft reset
  };

  // 0x840
  struct Reg_dm_cm_info1 : public Reg<Dm_cm_info1>
  {
    using Reg::Reg;
    // bit 17: H3(ES1.0), H3(ES1.1), M3W(E1.0) use this bit
    CXX_BITFIELD_MEMBER(20, 20, dtranend12, raw); ///< DMAC chan 1 transfer end
    CXX_BITFIELD_MEMBER(17, 17, dtranend11, raw); ///< DMAC chan 1 transfer end
    CXX_BITFIELD_MEMBER(16, 16, dtranend0, raw); ///< DMAC chan 0 transfer end
    CXX_BITFIELD_MEMBER(8, 8, segsuspend, raw);
    CXX_BITFIELD_MEMBER(0, 0, seqend, raw);
  };

  // 0x848
  struct Reg_dm_cm_info1_mask : public Reg<Dm_cm_info1_mask>
  {
    using Reg::Reg;
    explicit Reg_dm_cm_info1_mask() : Reg()
    {
      res1_7() = 0x7f;
      res9_15() = 0x7f;
      res18() = 1;
      res19() = 1;
      res21_31() = 0x7ff;
    }
    explicit Reg_dm_cm_info1_mask(l4_uint32_t v) : Reg_dm_cm_info1_mask()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(21, 31, res21_31, raw); ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(20, 20, dtranend12_mask, raw);
    CXX_BITFIELD_MEMBER(19, 19, res19, raw);    ///< must be always 1
    CXX_BITFIELD_MEMBER(18, 18, res18, raw);    ///< must be always 1
    CXX_BITFIELD_MEMBER(17, 17, dtranend11_mask, raw);
    CXX_BITFIELD_MEMBER(16, 16, dtranend0_mask, raw);
    CXX_BITFIELD_MEMBER(9, 15, res9_15, raw);   ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(8, 8, suspend_mask, raw);
    CXX_BITFIELD_MEMBER(1, 7, res1_7, raw);     ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(0, 0, seqend_mask, raw);

    void disable_ints()
    { raw = ~0U; }
  };

  // 0x850
  struct Reg_dm_cm_info2 : public Reg<Dm_cm_info2>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(17, 17, dtranerr1, raw); ///< DMAC chan 1 error
    CXX_BITFIELD_MEMBER(16, 16, dtranerr0, raw); ///< DMAC chan 0 error
    CXX_BITFIELD_MEMBER(0, 0, seqerr, raw);      ///< sequencer error
  };

  // 0x858
  struct Reg_dm_cm_info2_mask : public Reg<Dm_cm_info2_mask>
  {
    using Reg::Reg;
    explicit Reg_dm_cm_info2_mask() : Reg()
    {
      res1_15() = 0x7fff;
      res18_19() = 0x3;
      res20_31() = 0xfff;
    }
    explicit Reg_dm_cm_info2_mask(l4_uint32_t v) : Reg_dm_cm_info2_mask()
    { raw |= v; }

    CXX_BITFIELD_MEMBER(20, 31, res20_31, raw); ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(18, 19, res18_19, raw); ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(17, 17, dtranerr1_mask, raw);
    CXX_BITFIELD_MEMBER(16, 16, dtranerr0_mask, raw);
    CXX_BITFIELD_MEMBER(1, 15, res1_15, raw);   ///< all bits must be always 1
    CXX_BITFIELD_MEMBER(0, 0, seqerr_mask, raw);

    void disable_ints()
    { raw = ~0U; }
  };

  // 0x868
  struct Reg_dm_cm_seq_stat : public Reg<Dm_cm_seq_stat>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 1, seqtbsts, raw);   ///< busy flag of sequencer pr
  };

  // 0x880
  struct Reg_dm_dtran_addr : public Reg<Dm_dtran_addr>
  {
    using Reg::Reg;
    CXX_BITFIELD_MEMBER(0, 31, daddr, raw);     ///< address (8-byte aligned)
  };

  // 0x8a0
  struct Reg_dm_seq_cmd : public Reg<Dm_seq_cmd>
  {
    using Reg::Reg;
  };

  // 0x8a8
  struct Reg_dm_seq_arg : public Reg<Dm_seq_arg>
  {
    using Reg::Reg;
  };

  // 0x8b0
  struct Reg_dm_seq_size : public Reg<Dm_seq_size>
  {
    using Reg::Reg;
  };

  // 0x8b8
  struct Reg_dm_seq_seccnt : public Reg<Dm_seq_seccnt>
  {
    using Reg::Reg;
  };

  // 0x8c0
  struct Reg_dm_seq_rsp : public Reg<Dm_seq_rsp>
  {
    using Reg::Reg;
  };

  // 0x8c8
  struct Reg_dm_seq_rsp_chk : public Reg<Dm_seq_rsp_chk>
  {
    using Reg::Reg;
  };

  // 0x8d0
  struct Reg_dm_seq_addr : public Reg<Dm_seq_addr>
  {
    using Reg::Reg;
  };

  /** Wait until the controller is ready for command submission. */
  void cmd_wait_available(Cmd const *cmd, bool sleep);

  /** Wait until command phase has finished (interrupt wait loop). */
  void cmd_wait_cmd_finished(Cmd *cmd, bool verbose);

  /** Wait until data phase has finished (interrupt wait loop). */
  void cmd_wait_data_finished(Cmd *cmd);

  /** Fetch response from controller. */
  void cmd_fetch_response(Cmd *cmd);

  /** Submit command to controller. */
  void cmd_submit(Cmd *cmd);

  /** Handle interrupts related to the command phase. */
  void handle_irq_cmd(Cmd *cmd, Reg_sd_info sd_info);

  /** Handle interrupts related to the data phase. */
  void handle_irq_data(Cmd *cmd, Reg_sd_info sd_info);

  /** Disable clock when changing clock/timing. */
  void clock_disable();

  /** Enable clock after changing clock/timing. */
  void clock_enable();

  /** Set clock frequency. Clock should be disabled. */
  void set_clock(l4_uint32_t freq);

  /** Enable/disable DMA. */
  void enable_dma(bool enable);

private:
  Dbg warn;
  Dbg info;
  Dbg trace;
};

} // namespace Emmc
