/*
 * Copyright (C) 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Driver for SDHCI-like controllers.
 *
 * - SDHCI:
 * ``SD Specifications Part A2: SD Host Controller, Simplified Specification´´
 * - iproc/arasan:
 *  ``BROADCOM BCM2835 ARM Peripherals / External Mass Media Controller´´
 */

#pragma once

#include <string>

#include <l4/cxx/bitfield>
#include <l4/cxx/minmax>
#include <l4/sys/ktrace.h>

#include "debug.h"
#include "drv.h"
#include "inout_buffer.h"

class Bcm2835_mbox;

namespace Emmc {

enum class Sdhci_type
{
  Plain,   ///< Plain Sdhci driver.
  Usdhc,   ///< Sdhci driver with uSDHC modifications (NXP eSDHC i.MX).
  Iproc,   ///< Sdhci driver with iproc/arasan modifications.
  Bcm2711, /// Like iproc/arasan with bcm2711-specific modifications.
};

template <Sdhci_type TYPE>
class Sdhci : public Drv<Sdhci<TYPE>>
{
  friend Drv<Sdhci<TYPE>>;

  // Enable to generate kernel tracebuffer records for every SDHCI register
  // read/write access.
  enum { Trace_reg_access = false };

public:
  // XXX Can we avoid this?
  using Drv<Sdhci<TYPE>>::_dma_limit;
  using Drv<Sdhci<TYPE>>::_cmd_queue;
  using Drv<Sdhci<TYPE>>::_time_sleep;
  using Drv<Sdhci<TYPE>>::_bb_virt;
  using Drv<Sdhci<TYPE>>::_bb_phys;
  using Drv<Sdhci<TYPE>>::_bb_size;
  using Drv<Sdhci<TYPE>>::_receive_irq;
  using Drv<Sdhci<TYPE>>::_regs;
  using Drv<Sdhci<TYPE>>::provided_bounce_buffer;
  using Drv<Sdhci<TYPE>>::dma_accessible;
  using Drv<Sdhci<TYPE>>::delay;

public:
  static bool auto_cmd12()
  { return Auto_cmd12; }

  bool auto_cmd23() const
  {
    return Auto_cmd23
           && (   TYPE == Sdhci_type::Usdhc
               || TYPE == Sdhci_type::Iproc
               || TYPE == Sdhci_type::Bcm2711);
  }

  bool dma_adma2() const
  { return Dma_adma2; }

  static bool bounce_buffer_if_required()
  { return true; }

private:
  enum
  {
    /**
     * On true, suppress interrupts for command completion.
     *
     * There is no reason to trigger an interrupt for the completed command
     * execution if this command includes a data phase.
     */
    Suppress_cc_ints = true,

    /**
     * On true, use ADMA2 mode, otherwise use SDMA mode.
     *
     * With ADMA2 we use a single descriptor list for handling an entire
     * Block_device::Inout_block list. With SDMA we need to handle each of
     * those blocks with separate MMC commands.
     */
    Dma_adma2 = true,

    /**
     * On true, use the auto CMD12 feature.
     *
     * This automatically sends CMD12 after a transfer was finished. This is
     * necessary in case CMD23 is not available. Normally it's required to send
     * CMD12 manually but for certain uSDHC controllers this doesn't seem to
     * work (cf. Erratum ESDHC111). Hence, normally leave at \c true.
     */
    Auto_cmd12 = false,

    /**
     * On true, use the auto CMD23 feature.
     *
     * This saves the preceding CMD23 for a multi-read/write command and
     * the corresponding interrupt.
     *
     * Only for uSDHCI and iproc/arasan.
     */
    Auto_cmd23 = true,

    /**
     * On true, do not use DMA during setup for reading certain device
     * registers.
     *
     * Only for SDHCI.
     * If this is really necessary then something else is probably wrong.
     */
    No_dma_during_setup = false,
  };
  static_assert(!Auto_cmd23 || Dma_adma2, "Auto_cmd23 depends on Dma_adma2");

private:
  enum
  {
    /// true: use standard tuning feature (uSDHC only)
    Usdhc_std_tuning = true,
  };

  enum Regs
  {
    Ds_addr               = 0x00, ///< DMA System Address
    Blk_att               = 0x04, ///< Block Attributes (uSDHC)
    Blk_size              = 0x04, ///< Block Size Register (SDHCI)
    Cmd_arg               = 0x08, ///< Command Argument
    Cmd_xfr_typ           = 0x0c, ///< Command Transfer Type
    Cmd_rsp0              = 0x10, ///< Command Response 0
    Cmd_rsp1              = 0x14, ///< Command Response 1
    Cmd_rsp2              = 0x18, ///< Command Response 2
    Cmd_rsp3              = 0x1c, ///< Command Response 3
    Data_buff_acc_port    = 0x20, ///< Data Buffer Access Port
    Pres_state            = 0x24, ///< Present State
    Prot_ctrl             = 0x28, ///< Protocol Control (uSDHC)
    Host_ctrl             = 0x28, ///< Host Control (SDHCI)
    Sys_ctrl              = 0x2c, ///< System Control
    Int_status            = 0x30, ///< Interrupt Status
    Int_status_en         = 0x34, ///< Interrupt Status Enable
    Int_signal_en         = 0x38, ///< Interrupt Signal Enable
    Autocmd12_err_status  = 0x3c, ///< Auto CMD12 Error Status
    Host_ctrl2            = 0x3c, ///< Host Control 2 (iproc)
    Host_ctrl_cap         = 0x40, ///< Host Controller Capabilities (uSDHC)
    Cap1_sdhci            = 0x40, ///< Capabilities Register (SDHCI)
    Wtmk_lvl              = 0x44, ///< Watermark Level
    Cap2_sdhci            = 0x44, ///< Capabilities Register (SDHCI)
    Mix_ctrl              = 0x48, ///< Mixer Control
    Max_current           = 0x48, ///< Max Current Capabilities (SDHCI)
    Max_current2          = 0x4c, ///< Max Current Capabilities 2 (SDHCI)
    Force_event           = 0x50, ///< Force Event
    Adma_err_status       = 0x54, ///< ADMA Error Status
    Adma_sys_addr_lo      = 0x58, ///< ADMA System Address (lower 32-bit)
    Adma_sys_addr_hi      = 0x5c, ///< ADMA System Address (upper 32-bit)
    Dll_ctrl              = 0x60, ///< DLL (Delay Line) Control
    Dll_status            = 0x64, ///< DLL Status
    Clk_tune_ctrl_status  = 0x68, ///< CLK Tuning Control and Status
    Strobe_dll_ctrl       = 0x70, ///< Strobe DLL control
    Strobe_dll_status     = 0x74, ///< Strobe DLL status
    Vend_spec             = 0xc0, ///< Vendor Specific Register
    Mmc_boot              = 0xc4, ///< MMC Boot
    Vend_spec2            = 0xc8, ///< Vendor Specific 2 Register
    Tuning_ctrl           = 0xcc, ///< Tuning Control
    Host_version          = 0xfc, ///< Host Version (SDHCI)
    Cqe                  = 0x100, ///< Command Queue (see Cqe registers)
  };

  /**
   * Base class for `Reg<offs>` that implements an optional write delay
   */
  struct Reg_write_delay
  {
    static void write_delayed(Sdhci *sdhci, Regs offs, l4_uint32_t val);
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
  template <Regs offs>
  struct Reg : public Reg_write_delay
  {
    explicit Reg() : raw(0) {}
    explicit Reg(Sdhci<TYPE> const *sdhci)
    : raw(sdhci->_regs[offs])
    {
      if (Trace_reg_access)
        fiasco_tbuf_log_3val("read ", offs, raw, 0);
    }
    explicit Reg(l4_uint32_t v) : raw(v) {}
    l4_uint32_t read(Sdhci<TYPE> const *sdhci)
    {
      raw = sdhci->_regs[offs];
      if (Trace_reg_access)
        fiasco_tbuf_log_3val("read ", offs, raw, 0);
      return raw;
    }
    void write(Sdhci<TYPE> *sdhci)
    {
      if (Trace_reg_access)
        fiasco_tbuf_log_3val("WRITE", offs, raw, 0);
      Reg_write_delay::write_delayed(sdhci, offs, raw);
    }
    l4_uint32_t raw;
  };

  /// 0x00: DMA System Address
  struct Reg_ds_addr : public Reg<Ds_addr> { using Reg<Ds_addr>::Reg; };
  struct Reg_cmd_arg2 : public Reg<Ds_addr> { using Reg<Ds_addr>::Reg; };

  /// 0x04: uSDHC: Block Attributes
  struct Reg_blk_att : public Reg<Blk_att>
  {
    using Reg<Blk_att>::Reg;
    using Reg<Blk_att>::raw;

    CXX_BITFIELD_MEMBER(16, 31, blkcnt, raw); ///< Blocks count for transfer
    CXX_BITFIELD_MEMBER(0, 12, blksize, raw); ///< Transfer block size
  };

  /// 0x04: SDHCI: Block Size Register
  struct Reg_blk_size : public Reg<Blk_size>
  {
    using Reg<Blk_size>::Reg;
    using Reg<Blk_size>::raw;

    CXX_BITFIELD_MEMBER(16, 31, blkcnt, raw); ///< Blocks count for transfer
    CXX_BITFIELD_MEMBER(12, 14, sdma_buf_bndry, raw); ///< SDMA buffer boundary
    enum Sdma_boundary
    {
      Bndry_4k = 0,
      Bndry_8k = 1,
      Bndry_16k = 2,
      Bndry_32k = 3,
      Bndry_64k = 4,
      Bndry_128k = 5,
      Bndry_256k = 6,
      Bndry_512k = 7,
    };
    CXX_BITFIELD_MEMBER(0, 11, blksize, raw); ///< Transfer block size
  };

  /// 0x08: Command Argument
  struct Reg_cmd_arg : public Reg<Cmd_arg> { using Reg<Cmd_arg>::Reg; };

  /// 0x0c: Command Transfer Type
  struct Reg_cmd_xfr_typ : public Reg<Cmd_xfr_typ>
  {
    using Reg<Cmd_xfr_typ>::Reg;
    using Reg<Cmd_xfr_typ>::raw;

    CXX_BITFIELD_MEMBER(24, 29, cmdinx, raw); ///< Command index
    CXX_BITFIELD_MEMBER(22, 23, cmdtyp, raw); ///< Command type
    enum Cmd
    {
      Other = 0,
      Cmd52_suspend = 1,
      Cmd52_select = 2,
      Cmd52_abort = 3,
    };

    CXX_BITFIELD_MEMBER(21, 21, dpsel, raw);  ///< Data present select
    CXX_BITFIELD_MEMBER(20, 20, cicen, raw);  ///< Command index check enable
    CXX_BITFIELD_MEMBER(19, 19, cccen, raw);  ///< Command CRC check enable
    CXX_BITFIELD_MEMBER(18, 18, subcmd, raw); ///< Sub Command Flag
    CXX_BITFIELD_MEMBER(16, 17, rsptyp, raw); ///< Response type select
    enum Resp
    {
      No = 0,
      Length_136 = 1,
      Length_48 = 2,
      Length_48_check_busy = 3
    };

    // >>> SDHCI
    CXX_BITFIELD_MEMBER(8, 8, rspintdis, raw); ///< Response Interrupt Disable
    CXX_BITFIELD_MEMBER(7, 7, rspchk, raw);   ///< Response Error Check Enable
    CXX_BITFIELD_MEMBER(6, 6, r1r5, raw);     ///< 0=R1 (memory), 1=R5 (SDIO)
    CXX_BITFIELD_MEMBER(5, 5, msbsel, raw);   ///< multi/single block select
    CXX_BITFIELD_MEMBER(4, 4, dtdsel, raw);   ///< Data transfer direction
    CXX_BITFIELD_MEMBER(3, 3, ac23en, raw);   ///< Auto CMD23 enable
    CXX_BITFIELD_MEMBER(2, 2, ac12en, raw);   ///< Auto CMD12 enable
    CXX_BITFIELD_MEMBER(1, 1, bcen, raw);     ///< Block count enable
    CXX_BITFIELD_MEMBER(0, 0, dmaen, raw);    ///< DMA enable
    // <<< SDHCI
  };

  /// 0x10 .. 0x1c: Command response words
  struct Reg_cmd_rsp0 : public Reg<Cmd_rsp0> { using Reg<Cmd_rsp0>::Reg; };
  struct Reg_cmd_rsp1 : public Reg<Cmd_rsp1> { using Reg<Cmd_rsp1>::Reg; };
  struct Reg_cmd_rsp2 : public Reg<Cmd_rsp2> { using Reg<Cmd_rsp2>::Reg; };
  struct Reg_cmd_rsp3 : public Reg<Cmd_rsp3> { using Reg<Cmd_rsp3>::Reg; };

  /// 0x20: Buffer Data Port Register
  struct Reg_data_buff_acc_port : public Reg<Data_buff_acc_port>
  {
    using Reg<Data_buff_acc_port>::Reg;
  };

  /// 0x24: Present State
  struct Reg_pres_state : public Reg<Pres_state>
  {
    using Reg<Pres_state>::Reg;
    using Reg<Pres_state>::raw;

    // >>> uSDHC
    CXX_BITFIELD_MEMBER(31, 31, d7lsl, raw); ///< DATA7 line signal level
    CXX_BITFIELD_MEMBER(30, 30, d6lsl, raw); ///< DATA6 line signal level
    CXX_BITFIELD_MEMBER(29, 29, d5lsl, raw); ///< DATA5 line signal level
    CXX_BITFIELD_MEMBER(28, 28, d4lsl, raw); ///< DATA4 line signal level
    CXX_BITFIELD_MEMBER(27, 27, d3lsl, raw); ///< DATA3 line signal level
    CXX_BITFIELD_MEMBER(26, 26, d2lsl, raw); ///< DATA2 line signal level
    CXX_BITFIELD_MEMBER(25, 25, d1lsl, raw); ///< DATA1 line signal level
    CXX_BITFIELD_MEMBER(24, 24, d0lsl, raw); ///< DATA0 line signal level
    CXX_BITFIELD_MEMBER(24, 31, dlsl, raw);  ///< DATA[7:0] line signal level
    // <<< uSDHC
    // >>> SDHCI
    CXX_BITFIELD_MEMBER(28, 28, scs, raw);   ///< Sub command status
    CXX_BITFIELD_MEMBER(25, 25, hrvs, raw);  ///< Host regulator voltage stable
    // <<< SDHCI
    CXX_BITFIELD_MEMBER(24, 24, clsl, raw);  ///< CMD line signal level
    CXX_BITFIELD_MEMBER(20, 23, datlsl, raw);  ///< DAT line signal level
    CXX_BITFIELD_MEMBER(20, 20, dat0lsl, raw); ///< DAT[0] line signal level
    CXX_BITFIELD_MEMBER(19, 19, wpspl, raw); ///< Write protect switch pin level
    CXX_BITFIELD_MEMBER(18, 18, cdpl, raw);  ///< Card detect pin level
    CXX_BITFIELD_MEMBER(16, 16, cinst, raw); ///< Card inserted
    CXX_BITFIELD_MEMBER(15, 15, tscd, raw);  ///< Tape select change done
    CXX_BITFIELD_MEMBER(12, 12, rtr, raw);   ///< Re-tuning request
    CXX_BITFIELD_MEMBER(11, 11, bren, raw);  ///< Buffer read enable
    CXX_BITFIELD_MEMBER(10, 10, bwen, raw);  ///< Buffer write enable
    CXX_BITFIELD_MEMBER(9, 9, rta, raw);     ///< Read transfer active
    CXX_BITFIELD_MEMBER(8, 8, wta, raw);     ///< Write transfer active
    CXX_BITFIELD_MEMBER(7, 7, sdoff, raw);   ///< SD clock gated off internally
    CXX_BITFIELD_MEMBER(6, 6, peroff, raw);  ///< IPG_PERCLK gated off internally
    CXX_BITFIELD_MEMBER(5, 5, hckoff, raw);  ///< HCLK gated off internally
    CXX_BITFIELD_MEMBER(4, 4, ipgoff, raw);  ///< Peripheral clock gated off int
    CXX_BITFIELD_MEMBER(3, 3, sdstb, raw);   ///< SD clock stable
    CXX_BITFIELD_MEMBER(2, 2, dla, raw);     ///< Data line active
    CXX_BITFIELD_MEMBER(1, 1, cdihb, raw);   ///< Command inhibit (DATA)
    CXX_BITFIELD_MEMBER(0, 0, cihb, raw);    ///< Command inhibit (CMD)
  };

  /// 0x28: Protocol Control (uSDHC)
  struct Reg_prot_ctrl : public Reg<Prot_ctrl>
  {
    using Reg<Prot_ctrl>::Reg;
    using Reg<Prot_ctrl>::raw;

    CXX_BITFIELD_MEMBER(30, 30, non_exact_b_lk_rd, raw); ///< Non-exact block rd
    CXX_BITFIELD_MEMBER(27, 29, burst_len_en, raw); ///< BURST length enable
    /// Wakeup event enable on SD card removal
    CXX_BITFIELD_MEMBER(26, 26, wecrm, raw);
    /// Wakeup event enable on SD card insertion
    CXX_BITFIELD_MEMBER(25, 25, wecins, raw);
    /// Wakeup event enable on card interrupt
    CXX_BITFIELD_MEMBER(24, 24, wecint, raw);
    /// Read performed number 8 clock
    CXX_BITFIELD_MEMBER(20, 20, rd_done_no_8clk, raw);
    CXX_BITFIELD_MEMBER(19, 19, iabg, raw);    ///< Interrupt at block gap
    CXX_BITFIELD_MEMBER(18, 18, rwctl, raw);   ///< Read wait control
    CXX_BITFIELD_MEMBER(17, 17, creq, raw);    ///< Continue request
    CXX_BITFIELD_MEMBER(16, 16, sabgreq, raw); ///< Stop at block gap request
    CXX_BITFIELD_MEMBER(8, 9, dmasel, raw);    ///< DMA select
    enum
    {
      Dma_simple = 0,
      // ADMA1: data transfer of 4KiB-aligned data (see Adma1_desc).
      Dma_adma1 = 1,
      // ADMA2: data transfer of any location and any size (see Adma2_desc).
      Dma_adma2 = 2,
      // Reserved but other sources hint that this selects 64-bit Adam2 mode
      Dma_adma2_64 = 3,
    };
    CXX_BITFIELD_MEMBER(7, 7, cdss, raw);      ///< Card detect signal selection
    CXX_BITFIELD_MEMBER(6, 6, cdtl, raw);      ///< Card detect test level
    CXX_BITFIELD_MEMBER(4, 5, emode, raw);     ///< Endian mode
    enum
    {
      Endian_big = 0,
      Endian_big_half_word = 1,
      Endian_little = 2,
    };
    CXX_BITFIELD_MEMBER(3, 3, d3cd, raw);      ///< DATA3 as card detection pin
    CXX_BITFIELD_MEMBER(1, 2, dtw, raw);       ///< Data transfer width
    enum
    {
      Width_1bit = 0,
      Width_4bit = 1,
      Width_8bit = 2,
    };
    void set_bus_width(Mmc::Bus_width bus_width)
    {
      switch (bus_width)
        {
        case Mmc::Bus_width::Width_1bit: dtw() = Width_1bit; break;
        case Mmc::Bus_width::Width_4bit: dtw() = Width_4bit; break;
        case Mmc::Bus_width::Width_8bit: dtw() = Width_8bit; break;
        }
    }
    constexpr char const *str_bus_width() const
    {
      switch (dtw())
        {
        case Width_1bit: return "1-bit";
        case Width_4bit: return "4-bit";
        case Width_8bit: return "8-bit";
        default:         return "unknown";
        }
    }
    CXX_BITFIELD_MEMBER(0, 0, lctl, raw);      ///< LED control
  };

  /// 0x28: Host Control (SDHCI, iproc)
  struct Reg_host_ctrl : public Reg<Host_ctrl>
  {
    using Reg<Host_ctrl>::Reg;
    using Reg<Host_ctrl>::raw;

    CXX_BITFIELD_MEMBER(24, 31, wakeup, raw);
    CXX_BITFIELD_MEMBER(16, 23, gapctrl, raw);
    CXX_BITFIELD_MEMBER(13, 15, voltage_sel_vdd2, raw);
    CXX_BITFIELD_MEMBER(12, 12, bus_power_vdd2, raw); ///< SD bus power VDD2
    CXX_BITFIELD_MEMBER(9, 11, voltage_sel, raw); ///< SD bus volt select VDD1
    enum Voltage_mode
    {
      Voltage_33 = 7,
      Voltage_30 = 6,
      Voltage_18 = 5,
      Voltage_unsupported = 0,
    };
    CXX_BITFIELD_MEMBER(8, 8, bus_power, raw); ///< SD bus power VDD1
    CXX_BITFIELD_MEMBER(7, 7, cdtest_en, raw);
    CXX_BITFIELD_MEMBER(6, 6, cdtest_ins, raw);
    CXX_BITFIELD_MEMBER(5, 5, bbit8, raw);     ///< 8-bit bus
    CXX_BITFIELD_MEMBER(3, 4, dmamod, raw);    ///< DMA mode
    enum Dma_mode
    {
      Sdma = 0,
      Adma1 = 1,
      Adma32 = 2,
      Adma64 = 3,
    };
    CXX_BITFIELD_MEMBER(2, 2, hispd, raw);     ///< 1=high speed
    CXX_BITFIELD_MEMBER(1, 1, bbit4, raw);     ///< 4-bit bus
    enum
    {
      Width_1bit = 0,
      Width_4bit = 1,
      Width_8bit = 2,
    };
    void set_bus_width(Mmc::Bus_width bus_width)
    {
      switch (bus_width)
        {
        case Mmc::Bus_width::Width_1bit:
          bbit8() = 0;
          bbit4() = 0;
          break;
        case Mmc::Bus_width::Width_4bit:
          bbit8() = 0;
          bbit4() = 1;
          break;
        case Mmc::Bus_width::Width_8bit:
          bbit8() = 1;
          bbit4() = 0;
          break;
        }
    }
    constexpr char const *str_bus_width() const
    {
      if (bbit8())      return "8-bit";
      else if (bbit4()) return "4-bit";
      else              return "1-bit";
    }
    CXX_BITFIELD_MEMBER(0, 0, lctl, raw);      ///< LED control
  };

  /// 0x2c: System Control
  struct Reg_sys_ctrl : public Reg<Sys_ctrl>
  {
    using Reg<Sys_ctrl>::Reg;
    using Reg<Sys_ctrl>::raw;

    CXX_BITFIELD_MEMBER(28, 28, rstt, raw);   ///< Reset tuning
    CXX_BITFIELD_MEMBER(27, 27, inita, raw);  ///< Initialization active
    CXX_BITFIELD_MEMBER(26, 26, rstd, raw);   ///< Software reset
    CXX_BITFIELD_MEMBER(25, 25, rstc, raw);   ///< Software reset for CMD line
    CXX_BITFIELD_MEMBER(24, 24, rsta, raw);   ///< Software reset for all
    CXX_BITFIELD_MEMBER(23, 23, ipp_rst_n, raw); ///< Hardware reset
    CXX_BITFIELD_MEMBER(16, 19, dtocv, raw);  ///< Data timeout counter value
    enum Data_timeout
    {
      Sdclk_2_14 = 0,
      Sdclk_2_15 = 1,
      Sdclk_2_16 = 2,
      Sdclk_2_17 = 3,
      Sdclk_2_18 = 4,
      Sdclk_2_19 = 5,
      Sdclk_2_20 = 6,
      Sdclk_2_21 = 7,
      Sdclk_2_22 = 8,
      Sdclk_2_23 = 9,
      Sdclk_2_24 = 10,
      Sdclk_2_25 = 11,
      Sdclk_2_26 = 12,
      Sdclk_2_27 = 13,
      Sdclk_2_28 = 14,
      Sdclk_2_29 = 15,
      Sdclk_max  = Sdclk_2_29,
    };
    l4_uint32_t data_timeout_factor() const
    { return 1U << (14 + dtocv()); }

    // >>> uSDHC
    CXX_BITFIELD_MEMBER(8, 15, sdclkfs, raw); ///< SDCLK frequency select
    CXX_BITFIELD_MEMBER(4, 7, dvs, raw);      ///< Divisor
    l4_uint32_t clock_divider_sdr() const
    { return (sdclkfs() ? sdclkfs() * 2 : 1) * (dvs() + 1); }
    l4_uint32_t clock_divider_ddr() const
    { return (sdclkfs() ? sdclkfs() * 4 : 2) * (dvs() + 1); }
    // <<< uSDHC

    // >>> SD: 3.00, e.g. iproc
    CXX_BITFIELD_MEMBER(8, 15, clk_freq8, raw); ///< base divider LSBs
    CXX_BITFIELD_MEMBER(6, 7, clk_freq_ms2, raw); ///< base divider MSBs
    CXX_BITFIELD_MEMBER(5, 5, clk_gensel, raw); ///< mode of clock
    // 0=base_clock, 1=base_clock/2, 2=base_clock/4, 3=base_clock/6, ...
    l4_uint32_t clock_base_divider10() const
    { return (l4_uint32_t{clk_freq8()} + (clk_freq_ms2() << 8)) * 2; }
    // <<< SD: 3.00

    // >>> SDHCI
    CXX_BITFIELD_MEMBER(3, 3, pllen, raw);   ///< PLL enable
    CXX_BITFIELD_MEMBER(2, 2, sdcen, raw);   ///< SD clock enable
    CXX_BITFIELD_MEMBER(1, 1, icst, raw);    ///< Internal clock stable
    CXX_BITFIELD_MEMBER(0, 0, icen, raw);    ///< Internal clock enable
    // <<< SDHCI
  };

  /// 0x30: Interrupt Status
  struct Reg_int_status : public Reg<Int_status>
  {
    using Reg<Int_status>::Reg;
    using Reg<Int_status>::raw;

    CXX_BITFIELD_MEMBER(28, 28, dmae, raw);  ///< DMA error
    CXX_BITFIELD_MEMBER(26, 26, tne, raw);   ///< Tuning error
    CXX_BITFIELD_MEMBER(25, 25, admae, raw); ///< ADMA error (iproc)
    CXX_BITFIELD_MEMBER(24, 24, ac12e, raw); ///< Auto CMD12 error
    CXX_BITFIELD_MEMBER(23, 23, lime, raw);  ///< Limit error (iproc)
    CXX_BITFIELD_MEMBER(22, 22, debe, raw);  ///< Data end bit error
    CXX_BITFIELD_MEMBER(21, 21, dce, raw);   ///< Data CRC error
    CXX_BITFIELD_MEMBER(20, 20, dtoe, raw);  ///< Data timeout error
    CXX_BITFIELD_MEMBER(19, 19, cie, raw);   ///< Command index error
    CXX_BITFIELD_MEMBER(18, 18, cebe, raw);  ///< Command end bit error
    CXX_BITFIELD_MEMBER(17, 17, cce, raw);   ///< Command CRC error
    CXX_BITFIELD_MEMBER(16, 16, ctoe, raw);  ///< Command timeout error
    CXX_BITFIELD_MEMBER(15, 15, err, raw);   ///< An error has occurred (iproc)
    CXX_BITFIELD_MEMBER(14, 14, cqi, raw);   ///< Command queuing interrupt
    CXX_BITFIELD_MEMBER(13, 13, tp, raw);    ///< Tuning pass
    CXX_BITFIELD_MEMBER(12, 12, rte, raw);   ///< Re-tuning event
    CXX_BITFIELD_MEMBER(8, 8, cint, raw);    ///< Card interrupt
    CXX_BITFIELD_MEMBER(7, 7, crm, raw);     ///< Card removal
    CXX_BITFIELD_MEMBER(6, 6, cins, raw);    ///< Card insertion
    CXX_BITFIELD_MEMBER(5, 5, brr, raw);     ///< Buffer read ready
    CXX_BITFIELD_MEMBER(4, 4, bwr, raw);     ///< Buffer write ready
    CXX_BITFIELD_MEMBER(3, 3, dint, raw);    ///< DMA interrupt
    CXX_BITFIELD_MEMBER(2, 2, bge, raw);     ///< Block gap event
    CXX_BITFIELD_MEMBER(1, 1, tc, raw);      ///< Transfer complete
    CXX_BITFIELD_MEMBER(0, 0, cc, raw);      ///< Command complete

    /**
     * Return true if there was an error during command phase (command index
     * error, command end bit error, command CRC error).
     */
    bool cmd_error() const
    { return cie() || cebe() || cce(); }

    /**
     * Return true if there was an error during data phase (data end bit error,
     * data CRC error, data timeout error, DMA error).
     */
    bool data_error() const
    { return debe() || dce() || dtoe() || dmae() || admae(); }

    void copy_cmd_error(Reg_int_status const &other)
    {
      raw = 0;
      cie()  = other.cie();
      cebe() = other.cebe();
      cce()  = other.cce();
    }

    void copy_data_error(Reg_int_status const &other)
    {
      raw = 0;
      debe()  = other.debe();
      dce()   = other.dce();
      dtoe()  = other.dtoe();
      admae() = other.admae();
      dmae()  = other.dmae();
    }
  };

  /// 0x34: Interrupt Status Enable (SE)
  struct Reg_int_status_en : public Reg<Int_status_en>
  {
    using Reg<Int_status_en>::Reg;
    using Reg<Int_status_en>::raw;

    CXX_BITFIELD_MEMBER(28, 28, dmaesen, raw);  ///< DMA error SE
    CXX_BITFIELD_MEMBER(26, 26, tnesen, raw);   ///< Tuning error SE
    CXX_BITFIELD_MEMBER(25, 25, admaesen, raw); ///< ADMA error (iproc) SE
    CXX_BITFIELD_MEMBER(24, 24, ac12sene, raw); ///< Auto CMD12 error SE
    CXX_BITFIELD_MEMBER(23, 23, limesen, raw);  ///< Limit error (iproc) SE
    CXX_BITFIELD_MEMBER(22, 22, debesen, raw);  ///< Data end bit error SE
    CXX_BITFIELD_MEMBER(21, 21, dcesen, raw);   ///< Data CRC error SE
    CXX_BITFIELD_MEMBER(20, 20, dtoesen, raw);  ///< Data timeout error SE
    CXX_BITFIELD_MEMBER(19, 19, ciesen, raw);   ///< Command index error SE
    CXX_BITFIELD_MEMBER(18, 18, cebesen, raw);  ///< Command end it error SE
    CXX_BITFIELD_MEMBER(17, 17, ccesen, raw);   ///< Command CRC error SE
    CXX_BITFIELD_MEMBER(16, 16, ctoesen, raw);  ///< Command timeout error SE
    CXX_BITFIELD_MEMBER(14, 14, cqisen, raw);   ///< Command queuing intr SE
    CXX_BITFIELD_MEMBER(13, 13, tpsen, raw);    ///< Tuning pass SE
    CXX_BITFIELD_MEMBER(12, 12, rtesen, raw);   ///< Re-tuning event SE
    CXX_BITFIELD_MEMBER(8, 8, cintsen, raw);    ///< Card interrupt SE
    CXX_BITFIELD_MEMBER(7, 7, crmsen, raw);     ///< Card removal SE
    CXX_BITFIELD_MEMBER(6, 6, cinssen, raw);    ///< Card insertion SE
    CXX_BITFIELD_MEMBER(5, 5, brrsen, raw);     ///< Buffer read ready SE
    CXX_BITFIELD_MEMBER(4, 4, bwrsen, raw);     ///< Buffer write ready SE
    CXX_BITFIELD_MEMBER(3, 3, dintsen, raw);    ///< DMA interrupt SE
    CXX_BITFIELD_MEMBER(2, 2, bgesen, raw);     ///< Block gap event SE
    CXX_BITFIELD_MEMBER(1, 1, tcsen, raw);      ///< Transfer complete SE
    CXX_BITFIELD_MEMBER(0, 0, ccsen, raw);      ///< Command complete SE

    void enable_ints(Cmd const *cmd)
    {
      ccsen()    = 1;
      tcsen()    = 1;
      dintsen()  = 1;
      rtesen()   = 1;
      ctoesen()  = 1;
      ccesen()   = 1;
      cebesen()  = 1;
      ciesen()   = 1;
      dtoesen()  = 1;
      dcesen()   = 1;
      debesen()  = 1;
      limesen()  = 1;
      ac12sene() =    (Auto_cmd12 && cmd->flags.inout_cmd12())
                   || cmd->flags.auto_cmd23();
      admaesen() = 1;
      dmaesen()  = 1;
      brrsen()   =    cmd->cmd == Mmc::Cmd19_send_tuning_block
                   || cmd->cmd == Mmc::Cmd21_send_tuning_block;
    }
  };

  /// 0x38: Interrupt Signal Enable (IE)
  struct Reg_int_signal_en : public Reg<Int_signal_en>
  {
    using Reg<Int_signal_en>::Reg;
    using Reg<Int_signal_en>::raw;

    CXX_BITFIELD_MEMBER(28, 28, dmaeien, raw);  ///< DMA error status IE
    CXX_BITFIELD_MEMBER(26, 26, tneien, raw);   ///< Tuning error IE
    CXX_BITFIELD_MEMBER(25, 25, admaeien, raw); ///< ADMA error IE (iproc) IE
    CXX_BITFIELD_MEMBER(24, 24, ac12iene, raw); ///< Auto CMD12 error IE
    CXX_BITFIELD_MEMBER(23, 23, limeien, raw);  ///< Limit error IE (iproc) IE
    CXX_BITFIELD_MEMBER(22, 22, debeien, raw);  ///< Data end bit error IE
    CXX_BITFIELD_MEMBER(21, 21, dceien, raw);   ///< Data CRC error IE
    CXX_BITFIELD_MEMBER(20, 20, dtoeien, raw);  ///< Data timeout error IE
    CXX_BITFIELD_MEMBER(19, 19, cieien, raw);   ///< Command index error IE
    CXX_BITFIELD_MEMBER(18, 18, cebeien, raw);  ///< Command end it error IE
    CXX_BITFIELD_MEMBER(17, 17, cceien, raw);   ///< Command CRC error IE
    CXX_BITFIELD_MEMBER(16, 16, ctoeien, raw);  ///< Command timeout error IE
    CXX_BITFIELD_MEMBER(14, 14, cqiien, raw);   ///< Command queuing intr IE
    CXX_BITFIELD_MEMBER(13, 13, tpien, raw);    ///< Tuning pass IE
    CXX_BITFIELD_MEMBER(12, 12, rteien, raw);   ///< Re-tuning event IE
    CXX_BITFIELD_MEMBER(8, 8, cintien, raw);    ///< Card interrupt IE
    CXX_BITFIELD_MEMBER(7, 7, crmien, raw);     ///< Card removal IE
    CXX_BITFIELD_MEMBER(6, 6, cinsien, raw);    ///< Card insertion IE
    CXX_BITFIELD_MEMBER(5, 5, brrien, raw);     ///< Buffer read ready IE
    CXX_BITFIELD_MEMBER(4, 4, bwrien, raw);     ///< Buffer write ready IE
    CXX_BITFIELD_MEMBER(3, 3, dintien, raw);    ///< DMA interrupt IE
    CXX_BITFIELD_MEMBER(2, 2, bgeien, raw);     ///< Block gap event IE
    CXX_BITFIELD_MEMBER(1, 1, tcien, raw);      ///< Transfer complete IE
    CXX_BITFIELD_MEMBER(0, 0, ccien, raw);      ///< Command complete IE

    void enable_ints(Cmd const *cmd)
    {
      if (Suppress_cc_ints)
        ccien() = cmd->flags.has_data() ? 0 : 1;
      else
        ccien() = 1;
      tcien()    = 1;
      dintien()  = 1;
      rteien()   = 1;
      ctoeien()  = 1;
      cceien()   = 1;
      cceien()   = 1;
      cebeien()  = 1;
      cieien()   = 1;
      dtoeien()  = 1;
      dceien()   = 1;
      debeien()  = 1;
      limeien()  = 1;
      ac12iene() =    (Auto_cmd12 && cmd->flags.inout_cmd12())
                   || cmd->flags.auto_cmd23();
      admaeien() = 1;
      dmaeien()  = 1;
      brrien()   =    cmd->cmd == Mmc::Cmd19_send_tuning_block
                   || cmd->cmd == Mmc::Cmd21_send_tuning_block;
    }
  };

  /// 0x3c: Auto CMD12 Error Status
  struct Reg_autocmd12_err_status : public Reg<Autocmd12_err_status>
  {
    using Reg<Autocmd12_err_status>::Reg;
    using Reg<Autocmd12_err_status>::raw;

    CXX_BITFIELD_MEMBER(23, 23, smp_clk_sel, raw);    ///< Sample clock select
    CXX_BITFIELD_MEMBER(22, 22, execute_tuning, raw); ///< Execute tuning
    CXX_BITFIELD_MEMBER(7, 7, cnibac12e, raw);        ///< Command not issued by
    CXX_BITFIELD_MEMBER(4, 4, ac12ie, raw);  ///< Auto CMD12 / 23 index error
    CXX_BITFIELD_MEMBER(3, 3, ac12ce, raw);  ///< Auto CMD12 / 23 CRC error
    CXX_BITFIELD_MEMBER(2, 2, ac12ebe, raw); ///< Auto CMD12 / 23 end bit error
    CXX_BITFIELD_MEMBER(1, 1, ac12toe, raw); ///< Auto CMD12 / 23 timeout error
    CXX_BITFIELD_MEMBER(0, 0, ac12ne, raw);  ///< Auto CMD12 not executed
  };

  /// 0x3c: Control2 (iproc)
  struct Reg_host_ctrl2 : public Reg<Host_ctrl2>
  {
    using Reg<Host_ctrl2>::Reg;
    using Reg<Host_ctrl2>::raw;

    CXX_BITFIELD_MEMBER(31, 31, presvlen, raw);   ///< Preset value enable
    CXX_BITFIELD_MEMBER(30, 30, asyninten, raw);  ///< Asynchronous interrupt en
    CXX_BITFIELD_MEMBER(29, 29, bit64, raw);      ///< 64-bit addressing
    CXX_BITFIELD_MEMBER(28, 28, hostv4, raw);     ///< Host version 4 enable
    CXX_BITFIELD_MEMBER(27, 27, cmd23en, raw);    ///< CMD23 enable
    CXX_BITFIELD_MEMBER(26, 26, adma2len26, raw); ///< ADMA2 26-bit data length
    CXX_BITFIELD_MEMBER(24, 24, uhs2en, raw);     ///< UHS-II interface enable
    CXX_BITFIELD_MEMBER(23, 23, tuned, raw);
    CXX_BITFIELD_MEMBER(22, 22, tuneon, raw);
    CXX_BITFIELD_MEMBER(19, 19, v18, raw);        ///< 1.8V signaling enable
    CXX_BITFIELD_MEMBER(16, 18, uhsmode, raw);
    enum
    {
      Ctrl_uhs_sdr12 = 0,
      Ctrl_uhs_sdr25 = 1,
      Ctrl_uhs_sdr50 = 2,
      Ctrl_uhs_sdr104 = 3,
      Ctrl_uhs_ddr50 = 4,
      Ctrl_hs400 = 5,
    };
    CXX_BITFIELD_MEMBER(7, 7, notc12_err, raw); ///< Auto CMD12 error
    CXX_BITFIELD_MEMBER(4, 4, acbad_err, raw);  ///< ACMD: Command index error
    CXX_BITFIELD_MEMBER(3, 3, acend_err, raw);  ///< ACMD: End bit not 1
    CXX_BITFIELD_MEMBER(2, 2, accrc_err, raw);  ///< ACMD: Command CRC error
    CXX_BITFIELD_MEMBER(1, 1, acto_err, raw);   ///< ACMD: Timeout
    CXX_BITFIELD_MEMBER(0, 0, acnox_err, raw);  ///< ACMD: Not executed, error
  };

  /// 0x40: Host Controller Capabilities (uSDHC)
  /// i.MX8 QM: 0x07f3b407.
  struct Reg_host_ctrl_cap : public Reg<Host_ctrl_cap>
  {
    using Reg<Host_ctrl_cap>::Reg;
    using Reg<Host_ctrl_cap>::raw;

    CXX_BITFIELD_MEMBER(28, 28, bit64_v3, raw); ///< 64-bit system address V3
    CXX_BITFIELD_MEMBER(27, 27, bit64_v4, raw); ///< 64-bit system address V4
    CXX_BITFIELD_MEMBER(26, 26, vs18, raw);  ///< Voltage support 1.8V
    CXX_BITFIELD_MEMBER(25, 25, vs30, raw);  ///< Voltage support 3.0V
    CXX_BITFIELD_MEMBER(24, 24, vs33, raw);  ///< Voltage support 3.3V
    CXX_BITFIELD_MEMBER(23, 23, srs, raw);   ///< Suspend / resume support
    CXX_BITFIELD_MEMBER(22, 22, dmas, raw);  ///< DMA support
    CXX_BITFIELD_MEMBER(21, 21, hss, raw);   ///< High-speed support
    CXX_BITFIELD_MEMBER(20, 20, admas, raw); ///< ADMA support
    CXX_BITFIELD_MEMBER(16, 18, mbl, raw);   ///< Max block length
    CXX_BITFIELD_MEMBER(14, 15, retuning_mode, raw); ///< Retuning mode
    CXX_BITFIELD_MEMBER(13, 13, use_tuning_sdr50, raw); ///< Use tuning for SDR50
    ///< Time counter for retuning
    CXX_BITFIELD_MEMBER(8, 11, time_count_retuning, raw);
    CXX_BITFIELD_MEMBER(2, 2, ddr50_support, raw);  ///< DDR50 support
    CXX_BITFIELD_MEMBER(1, 1, sdr104_support, raw); ///< SDR104 support
    CXX_BITFIELD_MEMBER(0, 0, sdr50_support, raw);  ///< SDR50 support

    std::string str_caps() const
    {
      return std::string("vs18:") + std::to_string(vs18())
           + std::string(", vs30:") + std::to_string(vs30())
           + std::string(", vs33:") + std::to_string(vs33())
           + std::string(", sr:") + std::to_string(srs())
           + std::string(", dma:") + std::to_string(dmas())
           + std::string(", hs:") + std::to_string(hss())
           + std::string(", adma:") + std::to_string(admas())
           + std::string(", mbl:") + std::to_string(512 << mbl())
           + std::string(", tune:") + std::to_string(retuning_mode())
           + std::string(", ddr50:") + std::to_string(ddr50_support())
           + std::string(", sdr104:") + std::to_string(sdr104_support())
           + std::string(", sdr50:") + std::to_string(sdr50_support());
    }
  };

  /// 0x40: Host Controller Capabilities (SDHCI)
  struct Reg_cap1_sdhci : public Reg<Cap1_sdhci>
  {
    using Reg<Cap1_sdhci>::Reg;
    using Reg<Cap1_sdhci>::raw;

    CXX_BITFIELD_MEMBER(30, 31, slot_type, raw);
    CXX_BITFIELD_MEMBER(29, 29, async_int_support, raw);
    CXX_BITFIELD_MEMBER(28, 28, bit64_v3, raw); ///< 64-bit system address V3
    CXX_BITFIELD_MEMBER(27, 27, bit64_v4, raw); ///< 64-bit system address V4
    CXX_BITFIELD_MEMBER(26, 26, vs18, raw);     ///< Voltage support 1.8V
    CXX_BITFIELD_MEMBER(25, 25, vs30, raw);     ///< Voltage support 3.0V
    CXX_BITFIELD_MEMBER(24, 24, vs33, raw);     ///< Voltage support 3.3V
    CXX_BITFIELD_MEMBER(23, 23, srs, raw);      ///< Suspend / resume support
    CXX_BITFIELD_MEMBER(22, 22, dmas, raw);     ///< SDMA support
    CXX_BITFIELD_MEMBER(21, 21, hss, raw);      ///< High-speed support
    CXX_BITFIELD_MEMBER(19, 19, adma2s, raw);   ///< ADMA2 support
    CXX_BITFIELD_MEMBER(18, 18, bit8_bus, raw); ///< 8-bit support
    CXX_BITFIELD_MEMBER(16, 17, mbl, raw);      ///< Max block length
    CXX_BITFIELD_MEMBER(8, 15, base_freq, raw); ///< Base Clock Freq MHz for SD
    CXX_BITFIELD_MEMBER(7, 7, timeout_clock_unit, raw);
    CXX_BITFIELD_MEMBER(0, 5, timeout_clock_freq, raw);
  };

  /// 0x44: Watermark Level
  struct Reg_wtmk_lvl : public Reg<Wtmk_lvl>
  {
    using Reg<Wtmk_lvl>::Reg;
    using Reg<Wtmk_lvl>::raw;

    /// Write burst length due to system restriction
    CXX_BITFIELD_MEMBER(24, 28, wr_brst_len, raw);
    CXX_BITFIELD_MEMBER(16, 23, wr_wml, raw);  ///< Write watermark level
    /// Read burst length due to system restriction
    CXX_BITFIELD_MEMBER(8, 12, rd_brst_len, raw);
    CXX_BITFIELD_MEMBER(0, 7, rd_wml, raw);    ///< Read watermark level
    enum
    {
      Wml_dma = 64,     ///< Watermark level if DMA is active
      Brst_dma = 16,    ///< Burst length if DMA is active
    };
    static l4_uint32_t trunc_read(l4_uint32_t v) { return cxx::min(128U, v); }
    static l4_uint32_t trunc_write(l4_uint32_t v) { return cxx::min(128U, v); }
  };

  /// 0x44: Host Controller Capabilities (SDHCI)
  struct Reg_cap2_sdhci : public Reg<Cap2_sdhci>
  {
    using Reg<Cap2_sdhci>::Reg;
    using Reg<Cap2_sdhci>::raw;

    CXX_BITFIELD_MEMBER(28, 28, vdd2_18_support, raw);
    CXX_BITFIELD_MEMBER(27, 27, adma2_support, raw);
    CXX_BITFIELD_MEMBER(16, 23, clock_mult, raw);
    CXX_BITFIELD_MEMBER(14, 15, retune_modes, raw);
    enum
    {
      Tuning_mode_1 = 0,
      Tuning_mode_2 = 1,
      Tuning_mode_3 = 2,
    };
    CXX_BITFIELD_MEMBER(13, 13, tune_sdr50, raw);
    CXX_BITFIELD_MEMBER(8, 11, timer_count_retune, raw);
    CXX_BITFIELD_MEMBER(6, 6, driver_type_d_support, raw);
    CXX_BITFIELD_MEMBER(5, 5, driver_type_c_support, raw);
    CXX_BITFIELD_MEMBER(4, 4, driver_type_a_support, raw);
    CXX_BITFIELD_MEMBER(3, 3, uhs2_support, raw);
    CXX_BITFIELD_MEMBER(2, 2, ddr50_support, raw);
    CXX_BITFIELD_MEMBER(1, 1, sdr104_support, raw);
    CXX_BITFIELD_MEMBER(0, 0, sdr50_support, raw);
  };

  /// 0x48: Mixer Control (uSDHC)
  struct Reg_mix_ctrl : public Reg<Mix_ctrl>
  {
    using Reg<Mix_ctrl>::Reg;
    using Reg<Mix_ctrl>::raw;

    CXX_BITFIELD_MEMBER(27, 27, en_hs400_mo, raw); ///< Enable enhanced HS400
    CXX_BITFIELD_MEMBER(26, 26, hs400_mo, raw);   ///< Enable HS400 mode
    ///< Feedback clock source selection
    CXX_BITFIELD_MEMBER(25, 25, fbclk_sel, raw);
    CXX_BITFIELD_MEMBER(24, 24, auto_tune_en, raw); ///< Auto tuning enable
    CXX_BITFIELD_MEMBER(23, 23, smp_clk_sel, raw);  ///< Clock selection
    CXX_BITFIELD_MEMBER(22, 22, exe_tune, raw);   ///< Execute tuning
    CXX_BITFIELD_MEMBER(7, 7, ac23en, raw);       ///< Auto CMD23 enable
    CXX_BITFIELD_MEMBER(6, 6, nibble_pos, raw);   ///< Nibble position ind

    // >>> uSDHC
    CXX_BITFIELD_MEMBER(5, 5, msbsel, raw);       ///< multi/single block select
    CXX_BITFIELD_MEMBER(4, 4, dtdsel, raw);       ///< Data transfer direction
    CXX_BITFIELD_MEMBER(3, 3, ddr_en, raw);       ///< Dual data rate mode select
    CXX_BITFIELD_MEMBER(2, 2, ac12en, raw);       ///< Auto CMD12 enable
    CXX_BITFIELD_MEMBER(1, 1, bcen, raw);         ///< Block count enable
    CXX_BITFIELD_MEMBER(0, 0, dmaen, raw);        ///< DMA enable
    // <<< uSDHC
  };

  /// 0x48: Maximum Current Capabilities (SDHCI)
  struct Reg_max_current : public Reg<Max_current>
  {
    using Reg<Max_current>::Reg;
    using Reg<Max_current>::raw;

    CXX_BITFIELD_MEMBER(16, 23, max_current_18v_vdd1, raw);
    CXX_BITFIELD_MEMBER(8, 15, max_current_30v_vdd1, raw);
    CXX_BITFIELD_MEMBER(0, 7, max_current_33v_vdd1, raw);

    unsigned max_current(l4_uint8_t val)
    { return val * 4; }
  };

  /// 0x4c: Maximum Current Capabilities, bits 32-63 (SDHCI)
  struct Reg_max_current2 : public Reg<Max_current2>
  {
    using Reg<Max_current2>::Reg;
    using Reg<Max_current2>::raw;

    CXX_BITFIELD_MEMBER(0, 7, max_current_18v_vdd2, raw);
  };

  /// 0x54: ADMA Error status
  struct Reg_adma_err_status : public Reg<Adma_err_status>
  {
    using Reg<Adma_err_status>::Reg;
    using Reg<Adma_err_status>::raw;

    CXX_BITFIELD_MEMBER(3, 3, admadce, raw);
    CXX_BITFIELD_MEMBER(2, 2, adamlme, raw);
    CXX_BITFIELD_MEMBER(0, 1, adames, raw);
    enum
    {
      St_stop = 0,      ///< Stop DMA.
      St_fds = 1,       ///< Fetch descriptor.
      St_cadr = 2,      ///< Change address.
      St_tfr = 3,       ///< Transfer data.
    };
  };

  /// 0x58: ADMA System Address (64-bit)
  struct Reg_adma_sys_addr_lo : public Reg<Adma_sys_addr_lo>
  { using Reg<Adma_sys_addr_lo>::Reg; };
  struct Reg_adma_sys_addr_hi : public Reg<Adma_sys_addr_hi>
  { using Reg<Adma_sys_addr_hi>::Reg; };

  /// 0x60: DLL (Delay Line) Control
  struct Reg_dll_ctrl : public Reg<Dll_ctrl> { using Reg<Dll_ctrl>::Reg; };

  /// 0x68: CLK Tuning Control and Status
  struct Reg_clk_tune_ctrl_status : public Reg<Clk_tune_ctrl_status>
  {
    using Reg<Clk_tune_ctrl_status>::Reg;
    using Reg<Clk_tune_ctrl_status>::raw;

    CXX_BITFIELD_MEMBER(31, 31, pre_err, raw);
    CXX_BITFIELD_MEMBER(24, 30, tap_sel_pre, raw);
    CXX_BITFIELD_MEMBER(20, 23, tap_sel_out, raw);
    CXX_BITFIELD_MEMBER(16, 19, tap_sel_post, raw);
    CXX_BITFIELD_MEMBER(15, 15, nxt_err, raw);
    CXX_BITFIELD_MEMBER(8, 14, dly_cell_set_pre, raw);
    CXX_BITFIELD_MEMBER(4, 7, dly_cell_set_out, raw);
    CXX_BITFIELD_MEMBER(0, 3, dly_cell_set_post, raw);
  };

  /// 0x70: Strobe DLL control
  struct Reg_strobe_dll_ctrl : public Reg<Strobe_dll_ctrl>
  {
    using Reg<Strobe_dll_ctrl>::Reg;
    using Reg<Strobe_dll_ctrl>::raw;

    CXX_BITFIELD_MEMBER(28, 31, strobe_dll_ctrl_ref_update_int, raw);
    CXX_BITFIELD_MEMBER(20, 27, strobe_dll_ctrl_slv_update_int, raw);
    CXX_BITFIELD_MEMBER(9, 15, strobe_dll_ctrl_slv_override_val, raw);
    CXX_BITFIELD_MEMBER(8, 8, strobe_dll_ctrl_slv_override, raw);
    CXX_BITFIELD_MEMBER(7, 7, strobe_dll_ctrl_gate_update_1, raw);
    CXX_BITFIELD_MEMBER(6, 6, strobe_dll_ctrl_gate_update_0, raw);
    CXX_BITFIELD_MEMBER(3, 5, strobe_dll_ctrl_slv_dly_target, raw);
    CXX_BITFIELD_MEMBER(2, 2, strobe_dll_ctrl_slv_force_upd, raw);
    CXX_BITFIELD_MEMBER(1, 1, strobe_dll_ctrl_reset, raw);
    CXX_BITFIELD_MEMBER(0, 0, strobe_dll_ctrl_enable, raw);
  };

  /// 0x74: Strobe DLL status
  struct Reg_strobe_dll_status : public Reg<Strobe_dll_status>
  {
    using Reg<Strobe_dll_status>::Reg;
    using Reg<Strobe_dll_status>::raw;

    CXX_BITFIELD_MEMBER(9, 15, strobe_dll_sts_ref_sel, raw);
    CXX_BITFIELD_MEMBER(2, 8, strobe_dll_sts_slv_sel, raw);
    CXX_BITFIELD_MEMBER(1, 1, strobe_dll_sts_ref_lock, raw);
    CXX_BITFIELD_MEMBER(0, 0, strobe_dll_sts_slv_lock, raw);
  };

  /// 0xc0: Vendor Specific
  struct Reg_vend_spec : public Reg<Vend_spec>
  {
    using Reg<Vend_spec>::Reg;
    using Reg<Vend_spec>::raw;

    // >>> uSDHC
    CXX_BITFIELD_MEMBER(31, 31, cmd_byte_en, raw); ///< Byte access
    CXX_BITFIELD_MEMBER(15, 15, crc_chk_dis, raw); ///< CRC check disable
    CXX_BITFIELD_MEMBER(14, 14, cken, raw);        ///< XXX
    CXX_BITFIELD_MEMBER(13, 13, peren, raw);       ///< XXX
    CXX_BITFIELD_MEMBER(12, 12, hcken, raw);       ///< XXX
    CXX_BITFIELD_MEMBER(11, 11, ipgen, raw);       ///< XXX
    CXX_BITFIELD_MEMBER(8, 8, frc_sdclk_on, raw);  ///< Force CLK
    CXX_BITFIELD_MEMBER(3, 3, ac12_we_chk_busy_en, raw); ///< Check busy enbl
    CXX_BITFIELD_MEMBER(2, 2, conflict_check_en, raw); ///< Conflict check enbl
    CXX_BITFIELD_MEMBER(1, 1, vselect, raw);       ///< Voltage selection
    CXX_BITFIELD_MEMBER(0, 0, ext_dma_en, raw);    ///< External DMA request
    // <<< uSDHC

    enum { Default_bits = 0x24800000 };
  };

  /// 0xc4: MMC Boot
  struct Reg_mmc_boot : public Reg<Mmc_boot> { using Reg<Mmc_boot>::Reg; };

  /// 0xc8: Vendor Specific 2 Register
  struct Reg_vend_spec2 : public Reg<Vend_spec2>
  {
    using Reg<Vend_spec2>::Reg;
    using Reg<Vend_spec2>::raw;

    CXX_BITFIELD_MEMBER(16, 31, fbclk_tap_sel, raw);
    CXX_BITFIELD_MEMBER(15, 15, en_32k_clk, raw);
    CXX_BITFIELD_MEMBER(12, 12, acmd23_argu2_en, raw);
    CXX_BITFIELD_MEMBER(11, 11, hs400_rd_clk_stop_en, raw);
    CXX_BITFIELD_MEMBER(10, 10, hw400_wr_clk_stop_en, raw);
    CXX_BITFIELD_MEMBER(8, 8, en_busy_irq, raw);
    CXX_BITFIELD_MEMBER(6, 6, tuning_cmd_en, raw);
    CXX_BITFIELD_MEMBER(5, 5, tuning_1bit_en, raw);
    CXX_BITFIELD_MEMBER(4, 4, tuning_8bit_en, raw);
    CXX_BITFIELD_MEMBER(3, 3, card_int_d3_test, raw);
  };

  /// 0xcc: Tuning Control
  struct Reg_tuning_ctrl : public Reg<Tuning_ctrl>
  {
    using Reg<Tuning_ctrl>::Reg;
    using Reg<Tuning_ctrl>::raw;

    CXX_BITFIELD_MEMBER(24, 24, std_tuning_en, raw);
    CXX_BITFIELD_MEMBER(20, 22, tuning_window, raw);
    CXX_BITFIELD_MEMBER(16, 18, tuning_step, raw);
    CXX_BITFIELD_MEMBER(8, 15, tuning_counter, raw);
    // Not documented in NXP documentation
    CXX_BITFIELD_MEMBER(7, 7, disable_crc_on_tuning, raw);
    CXX_BITFIELD_MEMBER(0, 6, tuning_start_tap, raw);
  };

  /// 0xfe: SDHCI: Host Controller Version Register
  struct Reg_host_version : public Reg<Host_version>
  {
    using Reg<Host_version>::Reg;
    using Reg<Host_version>::raw;

    CXX_BITFIELD_MEMBER(24, 31, vend_vers, raw); ///< Vendor version number.
    CXX_BITFIELD_MEMBER(16, 23, spec_vers, raw); ///< Spec version number.
    constexpr char const *spec_version() const
    {
      switch (spec_vers())
        {
        case 0: return "1.00";
        case 1: return "2.00";
        case 2: return "3.00";
        case 3: return "4.00";
        case 4: return "4.10";
        case 5: return "4.20";
        default: return "> 4.20";
        }
    }
  };

  struct Adma1_desc
  {
    l4_uint32_t raw;
    CXX_BITFIELD_MEMBER(12, 31, addr, raw);
    CXX_BITFIELD_MEMBER(12, 27, length, raw); // 28-31 must be 0.
    CXX_BITFIELD_MEMBER(4, 5, act, raw);
    enum
    {
      Act_nop = 0,      ///< No operation.
      Act_set = 1,      ///< Set data length.
      Act_tran = 2,     ///< Transfer data.
      Act_link = 3,     ///< Link descriptor.
    };
    CXX_BITFIELD_MEMBER(2, 2, intr, raw);
    CXX_BITFIELD_MEMBER(1, 1, end, raw);
    CXX_BITFIELD_MEMBER(0, 0, valid, raw);
    void reset()
    { raw = 0; }
  };

  struct Adma2_desc_32
  {
    l4_uint32_t word0, word1;
    CXX_BITFIELD_MEMBER(16, 31, length, word0);
    CXX_BITFIELD_MEMBER(3, 5, act, word0);
    enum
    {
      Act_nop = 0,      ///< No operation.
      Act_rsv = 2,      ///< Reserved.
      Act_tran = 4,     ///< Transfer data.
      Act_link = 6,     ///< Link descriptor.
    };
    CXX_BITFIELD_MEMBER(2, 2, intr, word0);
    CXX_BITFIELD_MEMBER(1, 1, end, word0);
    CXX_BITFIELD_MEMBER(0, 0, valid, word0);

    void reset()
    { cxx::write_now(&word1, 0); cxx::write_now(&word0, 0); }

    L4Re::Dma_space::Dma_addr get_addr() const
    { return cxx::access_once(&word1); }

    void set_addr(L4Re::Dma_space::Dma_addr addr)
    { cxx::write_now(&word1, addr); }

    static L4Re::Dma_space::Dma_addr get_max_addr()
    { return ~0U; }
  };
  static_assert(sizeof(Adma2_desc_32) == 8, "Size of Adma2_desc_32!");

  struct Adma2_desc_64 : public Adma2_desc_32
  {
    // need to use cxx::access_once() / cxx::write_now() to prevent the compiler
    // from merging access into an unaligned 64-bit access to word2 + word3
    using Adma2_desc_32::word1;
    l4_uint32_t word2, word3;
    CXX_BITFIELD_MEMBER(0, 31, addr_hi, word2);
    void reset()
    {
      Adma2_desc_32::reset();
      cxx::write_now(&word2, 0);
    }

    L4Re::Dma_space::Dma_addr get_addr() const
    { return (L4Re::Dma_space::Dma_addr)cxx::access_once(&word2) << 32
             | cxx::access_once(&word1); }

    void set_addr(L4Re::Dma_space::Dma_addr addr)
    {
      cxx::write_now(&word1, addr & 0xffffffff);
      cxx::write_now(&word2, addr >> 32);
    }

    static L4Re::Dma_space::Dma_addr get_max_addr()
    { return ~0ULL; }
  };
  static_assert(sizeof(Adma2_desc_64) == 16, "Size of Adma2_desc_64!");

  static constexpr char const *type_name()
  {
    switch (TYPE)
      {
      case Sdhci_type::Plain:   return "SDHCI";
      case Sdhci_type::Usdhc:   return "uSDHC";
      case Sdhci_type::Iproc:   return "IProc";
      case Sdhci_type::Bcm2711: return "Bcm2711";
      default:      return "<unknown type>";
      }
  }

public:
  explicit Sdhci(int nr,
                 L4::Cap<L4Re::Dataspace> iocap,
                 L4::Cap<L4Re::Mmio_space> mmio_space,
                 l4_uint64_t mmio_base, l4_uint64_t mmio_size,
                 L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
                 l4_uint32_t host_clock, Receive_irq receive_irq);

  ~Sdhci();

  /** Initialize controller registers. */
  void init();

  /** IRQ handler. */
  Cmd *handle_irq();

  /** Disable all controller interrupts. */
  void mask_interrupts();

  /** Show interrupt status word if 'trace' debug level is enabled. */
  void show_interrupt_status(char const *s) const;

  /** Set clock and timing. */
  void set_clock_and_timing(l4_uint32_t clock, Mmc::Timing mmc_timing,
                            bool strobe = false);

  /** Set bus width. */
  void set_bus_width(Mmc::Bus_width bus_width);

  /** Set voltage (3.3V or 1.8V). */
  void set_voltage(Mmc::Voltage mmc_voltage);

  void set_voltage_18(bool enable);

  /** Return true if any of the UHS timings is supported by the controller. */
  bool supp_uhs_timings(Mmc::Timing timing) const
  {
    if (TYPE == Sdhci_type::Usdhc)
      {
        Reg_host_ctrl_cap cc(this);
        return    (timing & Mmc::Uhs_sdr12) // always supported
               || (timing & Mmc::Uhs_sdr25) // always supported
               || ((timing & Mmc::Uhs_sdr50) && cc.sdr50_support())
               || ((timing & Mmc::Uhs_sdr104) && cc.sdr104_support())
               || ((timing & Mmc::Uhs_ddr50) && cc.ddr50_support());
      }
    else
      {
        Reg_cap2_sdhci c2(this);
        return    (timing & Mmc::Uhs_sdr12) // always supported
               || (timing & Mmc::Uhs_sdr25) // always supported
               || ((timing & Mmc::Uhs_sdr50) && c2.sdr50_support())
               || ((timing & Mmc::Uhs_sdr104) && c2.sdr104_support())
               || ((timing & Mmc::Uhs_ddr50) && c2.ddr50_support());
      }
  };

  /** Return true if the selected timing needs tuning. */
  bool needs_tuning_sdr50() const
  {
    if (TYPE == Sdhci_type::Usdhc)
      {
        Reg_host_ctrl_cap cc(this);
        return cc.use_tuning_sdr50();
      }
    else
      {
        Reg_cap2_sdhci c2(this);
        return c2.tune_sdr50();
      }
  }

  /** Return true if the power limit is supported by the controller. */
  constexpr bool supp_power_limit(Mmc::Power_limit power) const
  {
    switch (power)
      {
      case Mmc::Power_072w:
      case Mmc::Power_144w:
      case Mmc::Power_216w:
      case Mmc::Power_288w:
        return true;
      case Mmc::Power_180w:
      default:
        return false;
      }
  }

  /** Return true if tuning has finished. */
  bool tuning_finished(bool *success);

  void reset_tuning();
  void enable_auto_tuning();

  /** Return true if the card is busy. */
  constexpr bool card_busy() const
  {
    switch (TYPE)
      {
      case Sdhci_type::Iproc:
      case Sdhci_type::Bcm2711:
        return !Reg_pres_state(this).dat0lsl();
      default:
        return !Reg_pres_state(this).d0lsl();
      }
  }

  /** Return supported power values by the controller. */
  Mmc::Reg_ocr supported_voltage() const;

  /**
   * Return true if the controller supports up to 540mW at the desired voltage.
   */
  bool xpc_supported(Mmc::Voltage voltage) const;

  /** Dump all controller registers if 'warn' debug level is enabled. */
  void dump() const;

private:
  /**
   * Wait until the controller is ready for command submission.
   *
   * It depends on the nature of the (following) command which condition
   * needs to be tested. The \c cmd parameter provides this information.
   */
  void cmd_wait_available(Cmd const *cmd, bool sleep);

  /** Wait until command phase has finished (interrupt wait loop). */
  void cmd_wait_cmd_finished(Cmd *cmd, bool verbose);

  /** Wait until data phase has finished (interrupt wait loop). */
  void cmd_wait_data_finished(Cmd *cmd);

  /** Fetch response from controller. */
  void cmd_fetch_response(Cmd *cmd);

  /** Return string containing controller capabilities. */
  std::string str_caps() const
  { return Reg_host_ctrl_cap(this).str_caps(); }

  /** Submit command to controller. */
  void cmd_submit(Cmd *cmd);

  /** Handle interrupts related to the command phase. */
  void handle_irq_cmd(Cmd *cmd, Reg_int_status is);

  /** Handle interrupts related to the data phase. */
  void handle_irq_data(Cmd *cmd, Reg_int_status is);

  /** Disable clock when changing clock/timing. */
  void clock_disable();

  /** Enable clock after changing clock/timing. */
  void clock_enable();

  /** Set clock frequency. Clock should be disabled. */
  void set_clock(l4_uint32_t freq);

  /** Required for HS400. */
  void set_strobe_dll();

  /** Set ADMA2 descriptors for a single memory region. */
  template<typename T>
  T* adma2_set_descs_mem_region(T *desc, l4_uint64_t b_addr,
                                l4_uint32_t b_size, bool terminate = true);

  /** Set ADMA2 descriptors using inout() block request. */
  template<typename T>
  void adma2_set_descs(T *descs, Cmd *cmd);
  void adma2_set_descs_blocks(Cmd *cmd);

  /** Set ADMA2 descriptor using physical address + length (CMD8). */
  void adma2_set_descs_memory_region(l4_addr_t phys, l4_uint32_t size);

  /** Dump ADMA2 descriptors. */
  template<typename T>
  void adma2_dump_descs(T const *desc) const;
  void adma2_dump_descs() const;

  // ::::: Platform-specific :::::
  void init_platform(L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma);
  void done_platform();
  // :::::::::::::::::::::::::::::

  Inout_buffer _adma2_desc_mem;         ///< Dataspace for descriptor memory.
  L4Re::Dma_space::Dma_addr _adma2_desc_phys; ///< Physical address of ADMA2 descs.
  Adma2_desc_64 *_adma2_desc;           ///< ADMA2 descriptor list (32/64-bit).
  l4_addr_t _dma_offset = 0;            ///< DMA offset (bcm2835)
  Bcm2835_mbox *bcm2835_mbox = nullptr; ///< For iproc: SoC control over mailbox
  bool _ddr_active = false;             ///< True if double-data timing.
  bool _adma2_64 = false;               ///< True if 64-bit ADMA2.
  l4_uint32_t _host_clock;              ///< Reference clock frequency.

  Dbg warn;
  Dbg info;
  Dbg trace;
  Dbg trace2;

  /**
   * Wait until _write_delay microseconds have been passed since the last write
   * operation.
   */
  void write_delay()
  {
    Util::busy_wait_until(_write_delay_last_reg_write + _write_delay);
  }

  /**
   * Record the time when the last write operation has been performed.
   */
  void update_last_write()
  {
    _write_delay_last_reg_write = Util::tsc_to_us(Util::read_tsc());
  }

  l4_uint32_t _write_delay = 0;
  l4_uint64_t _write_delay_last_reg_write = 0;
}; // class Sdhci

} // namespace Emmc
