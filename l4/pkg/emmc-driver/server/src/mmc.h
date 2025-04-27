/*
 * Copyright (C) 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/cxx/bitfield>
#include <string>

namespace Emmc {

/**
 * eMMC command context.
 */
class Mmc
{
public:
  /// Command type.
  enum
  {
    Bc   = 0 << 6,      ///< broadcast commands, no response
    Bcr  = 1 << 6,      ///< broadcast command, response from all cards
    Ac   = 2 << 6,      ///< addressed commands, no data transfer on the DATA
    Adtc = 3 << 6,      ///< addressed data transfer commands
  };

  /// Command features.
  enum
  {
    Idx_mask       = 0x3f,    ///< command index part
    Type_mask      = 0xc0,    ///< command type
    Rsp_none       = 0,
    Rsp_present    = 1 <<  8, ///< response expected
    Rsp_check_crc  = 1 <<  9, ///< check CRC
    Rsp_136_bits   = 1 << 10, ///< response length of 136 bits
    Rsp_check_busy = 1 << 11, ///< check busy
    Rsp_has_opcode = 1 << 12, ///< response sets command index
    Rsp_mask       = Rsp_present | Rsp_check_crc | Rsp_136_bits
                   | Rsp_check_busy | Rsp_has_opcode,
    Dir_read       = 1 << 13, ///< command transfers from device to host
  };

  /// Response types.
  enum
  {
    Resp_r1  = Rsp_present | Rsp_check_crc | Rsp_has_opcode,
    Resp_r1b = Rsp_present | Rsp_check_crc | Rsp_has_opcode | Rsp_check_busy,
    Resp_r2  = Rsp_present | Rsp_136_bits | Rsp_check_crc,
    Resp_r3  = Rsp_present,
    Resp_r4  = Rsp_present,
    Resp_r5  = Rsp_present | Rsp_check_crc | Rsp_has_opcode,
    Resp_r6  = Rsp_present | Rsp_check_crc | Rsp_has_opcode,
    Resp_r7  = Rsp_present | Rsp_check_crc | Rsp_has_opcode,
  };

  /// eMMC commands.
  enum : l4_uint32_t
  {
    Cmd0_go_idle_state          =  0 | Bc,
    Cmd1_send_op_cond           =  1 | Bcr  | Resp_r3  | Dir_read,
    Cmd2_all_send_cid           =  2 | Bcr  | Resp_r2,
    Cmd3_send_relative_addr     =  3 | Ac   | Resp_r6,             // SD
    Cmd3_set_relative_addr      =  3 | Ac   | Resp_r1,             // MMC
    Cmd4_set_dsr                =  4 | Bc,
    Cmd5_io_send_op_cond        =  5 | Bc   | Resp_r4,             // SD
    Cmd5_sleep_awake            =  5 | Ac   | Resp_r1b,            // MMC
    Cmd6_switch_func            =  6 | Adtc | Resp_r1  | Dir_read, // SD
    Cmd6_switch                 =  6 | Ac   | Resp_r1b | Dir_read, // MMC
    // XXX R1 while selecting from Stand-by State to Transfer State.
    //     R1b while selecting from Disconnected State to Programming State.
    Cmd7_select_card            =  7 | Ac   | Resp_r1,
    Cmd8_send_ext_csd           =  8 | Adtc | Resp_r1  | Dir_read,
    Cmd8_send_if_cond           =  8 | Bcr  | Resp_r7,
    Cmd9_send_csd               =  9 | Ac   | Resp_r2,
    Cmd10_send_cid              = 10 | Ac   | Resp_r2,
    Cmd11_read_dat_until_stop   = 11 | Adtc | Resp_r1,
    Cmd11_voltage_switch        = 11 | Ac   | Resp_r1,
    Cmd12_stop_transmission_rd  = 12 | Ac   | Resp_r1,             // read
    Cmd12_stop_transmission_wr  = 12 | Ac   | Resp_r1b,            // write
    Cmd13_send_status           = 13 | Ac   | Resp_r1  | Dir_read, // MMC
    Cmd15_go_inactive_state     = 15 | Ac,
    Cmd16_set_blocklen          = 16 | Ac   | Resp_r1,
    Cmd17_read_single_block     = 17 | Adtc | Resp_r1  | Dir_read,
    Cmd18_read_multiple_block   = 18 | Adtc | Resp_r1  | Dir_read,
    Cmd19_send_tuning_block     = 19 | Adtc | Resp_r1  | Dir_read, // SD
    Cmd20_write_dat_until_stop  = 20 | Adtc | Resp_r1,
    Cmd21_send_tuning_block     = 21 | Adtc | Resp_r1  | Dir_read, // MMC
    Cmd22_address_extension     = 22 | Ac   | Resp_r1,             // SDUC
    Cmd23_set_block_count       = 23 | Ac   | Resp_r1,
    Cmd24_write_block           = 24 | Adtc | Resp_r1,
    Cmd25_write_multiple_block  = 25 | Adtc | Resp_r1,
    Cmd26_program_cid           = 26 | Adtc | Resp_r1,
    Cmd27_program_csd           = 27 | Adtc | Resp_r1,
    Cmd28_set_write_prot        = 28 | Ac   | Resp_r1b,
    Cmd29_clr_write_prot        = 29 | Ac   | Resp_r1b,
    Cmd30_send_write_prot       = 30 | Adtc | Resp_r1  | Dir_read,
    Cmd32_tag_sector_start      = 32 | Ac   | Resp_r1,
    Cmd33_tag_sector_end        = 33 | Ac   | Resp_r1,
    Cmd34_untag_sector          = 34 | Ac   | Resp_r1,
    Cmd35_tag_erase_group_start = 35 | Ac   | Resp_r1,
    Cmd36_tag_erase_group_end   = 36 | Ac   | Resp_r1,
    Cmd37_untag_erase_group     = 37 | Ac   | Resp_r1,
    Cmd38_erase                 = 38 | Ac   | Resp_r1b,
    Cmd39_fast_io               = 39 | Ac   | Resp_r4,
    Cmd40_go_irq_state          = 40 | Bcr  | Resp_r5,
    Cmd42_lock_unlock           = 42 | Adtc | Resp_r1b,
    Cmd52_io_rw_direct          = 52 | Ac   | Resp_r5,             // SD
    Cmd53_io_rw_extended        = 53 | Ac   | Resp_r5,
    Cmd55_app_cmd               = 55 | Ac   | Resp_r1,
    Cmd56_gen_cmd               = 56 | Adtc | Resp_r1b,
    Cmd60_rw_multiple_register  = 60 | Adtc | Resp_r1b,
    Cmd61_rw_multiple_block     = 61 | Adtc | Resp_r1b,

    // always come after Cmd55_app_cmd
    Acmd6_set_bus_width         =  6 | Ac   | Resp_r1,
    Acmd13_sd_status            = 13 | Adtc | Resp_r1  | Dir_read,
    Acmd22_send_num_wr_sectors  = 22 | Adtc | Resp_r1  | Dir_read,
    Acmd23_set_wr_blk_erase_cnt = 23 | Ac   | Resp_r1,
    Acmd41_sd_app_op_cond       = 41 | Bcr  | Resp_r3,
    Acmd42_set_clr_card_detect  = 42 | Ac   | Resp_r1,
    Acmd51_send_scr             = 51 | Adtc | Resp_r1  | Dir_read,
  };

  enum Bus_width
  {
    Width_1bit,
    Width_4bit,
    Width_8bit,
  };

  enum Timing
  {
    Legacy = 0,
    Hs = 1 << 0,
    Uhs_sdr12 = 1 << 1,
    Uhs_sdr25 = 1 << 2,
    Uhs_sdr50 = 1 << 3,
    Uhs_sdr104 = 1 << 4,
    Uhs_ddr50 = 1 << 5,
    Mmc_ddr52 = 1 << 6,
    Mmc_hs200 = 1 << 7,
    Mmc_hs400 = 1 << 8,
    Uhs_modes = Uhs_sdr12 | Uhs_sdr25 | Uhs_sdr50 | Uhs_sdr104 | Uhs_ddr50
  };

  enum Voltage
  {
    Voltage_120, // 1.2V
    Voltage_180, // 1.8V
    Voltage_330, // 3.3V
  };

  enum Power_limit
  {
    Power_072w, // 200mA
    Power_144w, // 400mA
    Power_216w, // 600mA
    Power_288w, // 800mA
    Power_180w, // 400mA
  };

  static char const *str_timing(Timing timing)
  {
    switch (timing)
      {
      case Legacy:     return "Legacy";
      case Hs:         return "High-Speed";
      case Uhs_sdr12:  return "UHS SDR12";
      case Uhs_sdr25:  return "UHS SDR25";
      case Uhs_sdr50:  return "UHS SDR50";
      case Uhs_sdr104: return "UHS SDR104";
      case Uhs_ddr50:  return "UHS DDR50";
      case Mmc_ddr52:  return "MMC DDR52";
      case Mmc_hs200:  return "MMC HS200";
      case Mmc_hs400:  return "MMC HS400";
      default:         return "unknown";
      }
  }

  static char const *str_voltage(Voltage voltage)
  {
    switch (voltage)
      {
      case Voltage_120: return "1.2V";
      case Voltage_180: return "1.8V";
      case Voltage_330: return "3.3V";
      default:          return "unknown";
      }
  }

  struct Device_status
  {
    explicit Device_status(l4_uint32_t r)
    : raw(r)
    {}

    l4_uint32_t raw;

    // A: bits are set and cleared in accordance with the device status
    // B: bits are cleared as soon as the response (error report) is sent out
    CXX_BITFIELD_MEMBER(31, 31, address_out_of_range, raw);  // B: 1=error
    CXX_BITFIELD_MEMBER(30, 30, address_misalign, raw);      // B: 1=error
    CXX_BITFIELD_MEMBER(29, 29, block_len_error, raw);       // B: 1=error
    CXX_BITFIELD_MEMBER(28, 28, erase_seq_error, raw);       // B: 1=error
    CXX_BITFIELD_MEMBER(27, 27, erase_param, raw);           // B: 1=error
    CXX_BITFIELD_MEMBER(26, 26, wp_violation, raw);          // B: 1=error
    CXX_BITFIELD_MEMBER(25, 25, device_is_locked, raw);      // A: 1=locked
    CXX_BITFIELD_MEMBER(24, 24, lock_unlock_failed, raw);    // B: 1=error
    CXX_BITFIELD_MEMBER(23, 24, com_crc_error, raw);         // B: 1=error
    CXX_BITFIELD_MEMBER(22, 22, illegal_command, raw);       // B: 1=error
    CXX_BITFIELD_MEMBER(21, 21, device_ecc_failed, raw);     // B: 1=failure
    CXX_BITFIELD_MEMBER(20, 20, cc_error, raw);              // B: 1=error
    CXX_BITFIELD_MEMBER(19, 19, error, raw);                 // B: 1=error
    CXX_BITFIELD_MEMBER(16, 16, cid_csd_overwrite, raw);     // B: 1=error
    CXX_BITFIELD_MEMBER(15, 15, wp_erase_skip, raw);         // B: 1=protected
    CXX_BITFIELD_MEMBER(13, 13, erase_reset, raw);           // B: 1=set
    CXX_BITFIELD_MEMBER(9, 12, current_state, raw);          // A: status
    enum State
    {
      Idle = 0,
      Ready = 1,
      Identification = 2,
      Standby = 3,
      Transfer = 4,
      Data_send = 5,
      Data_receive = 6,
      Programming = 7,
      Disconnect = 8,
      Bus_test = 9,
      Sleep = 10,
    };
    char const *str() const
    {
      switch (current_state())
        {
        case Idle:           return "Idle";
        case Ready:          return "Ready";
        case Identification: return "Identification";
        case Standby:        return "Standby";
        case Transfer:       return "Transfer";
        case Data_send:      return "Data send";
        case Data_receive:   return "Data receive";
        case Programming:    return "Programming";
        case Disconnect:     return "Disconnect";
        case Bus_test:       return "Bus test";
        case Sleep:          return "Sleep";
        default:             return "unknown";
        }
    }
    CXX_BITFIELD_MEMBER(8, 8, ready_for_data, raw);          // A: 1=ready
    CXX_BITFIELD_MEMBER(7, 7, switch_error, raw);            // B: 1=error
    CXX_BITFIELD_MEMBER(6, 6, exception_event, raw);         // A: 1=exception
    CXX_BITFIELD_MEMBER(5, 5, app_cmd, raw);                 // A: 1=enabled
    bool error_condition() const
    {
      return exception_event() || switch_error() || error() || cc_error()
          || illegal_command() || com_crc_error() || lock_unlock_failed()
          || device_is_locked() || wp_violation() || erase_param()
          || block_len_error() || address_misalign() || address_out_of_range();
    }
  };

  /**
   * 136-bit answer.
   * Sent in response to CDM2 (ALL_SEND_CID) and CMD10 (SEND_CID).
   */
  struct Reg_cid
  {
    explicit Reg_cid(l4_uint32_t const r[4])
    {
      for (unsigned i = 0; i < 4; ++i)
        raw[i] = r[i];
    }

    union
    {
      struct { l4_uint32_t raw[4]; };

      struct
      {
        l4_uint32_t raw0, raw1, raw2, raw3;
        CXX_BITFIELD_MEMBER(24, 31,  mid, raw0);
        CXX_BITFIELD_MEMBER(16, 17,  cbx, raw0);
        CXX_BITFIELD_MEMBER( 8, 15,  oid, raw0);
        CXX_BITFIELD_MEMBER( 0,  7, pnm0, raw0);
        CXX_BITFIELD_MEMBER(24, 31, pnm1, raw1);
        CXX_BITFIELD_MEMBER(16, 23, pnm2, raw1);
        CXX_BITFIELD_MEMBER( 8, 15, pnm3, raw1);
        CXX_BITFIELD_MEMBER( 0,  7, pnm4, raw1);
        CXX_BITFIELD_MEMBER(24, 31, pnm5, raw2);
        CXX_BITFIELD_MEMBER(16, 23,  prv, raw2);
        CXX_BITFIELD_MEMBER( 8, 15, psn0, raw2);
        CXX_BITFIELD_MEMBER( 0,  7, psn1, raw2);
        CXX_BITFIELD_MEMBER(24, 31, psn2, raw3);
        CXX_BITFIELD_MEMBER(16, 23, psn3, raw3);
        CXX_BITFIELD_MEMBER( 8, 15,  mdt, raw3);
        CXX_BITFIELD_MEMBER( 1,  7,  crc, raw3);
        std::string pnm() const
        {
          return std::string("") + (char)pnm0() + (char)pnm1() + (char)pnm2()
                                 + (char)pnm3() + (char)pnm4() + (char)pnm5();
        }
        l4_uint32_t psn() const
        {
          return   ((l4_uint32_t)psn0() << 24) | ((l4_uint32_t)psn1() << 16)
                 | ((l4_uint32_t)psn2() <<  8) | ((l4_uint32_t)psn3() <<  0);
        }
        // We assume eMMC >= 4.41 but should actually check EXT_CSD[192]
        l4_uint32_t myr() const { return 2013 + (mdt() & 0xf); }
        l4_uint32_t mmth() const { return mdt() >> 4; }
      } mmc;
      struct
      {
        l4_uint32_t raw0, raw1, raw2, raw3;
        CXX_BITFIELD_MEMBER(24, 31,  mid, raw0);
        CXX_BITFIELD_MEMBER( 8, 23,  oid, raw0);
        CXX_BITFIELD_MEMBER( 0,  7, pnm0, raw0);
        CXX_BITFIELD_MEMBER(24, 31, pnm1, raw1);
        CXX_BITFIELD_MEMBER(16, 23, pnm2, raw1);
        CXX_BITFIELD_MEMBER( 8, 15, pnm3, raw1);
        CXX_BITFIELD_MEMBER( 0,  7, pnm4, raw1);
        CXX_BITFIELD_MEMBER(24, 31,  prv, raw2);
        CXX_BITFIELD_MEMBER(16, 23, psn0, raw2);
        CXX_BITFIELD_MEMBER( 8, 15, psn1, raw2);
        CXX_BITFIELD_MEMBER( 0,  7, psn2, raw2);
        CXX_BITFIELD_MEMBER(24, 31, psn3, raw3);
        CXX_BITFIELD_MEMBER( 8, 19,  mdt, raw3);
        CXX_BITFIELD_MEMBER( 1,  7,  crc, raw3);
        std::string pnm() const
        {
          return std::string("") + (char)pnm0() + (char)pnm1() + (char)pnm2()
                                 + (char)pnm3() + (char)pnm4();
        }
        l4_uint32_t psn() const
        {
          return   ((l4_uint32_t)psn0() << 24) | ((l4_uint32_t)psn1() << 16)
                 | ((l4_uint32_t)psn2() <<  8) | ((l4_uint32_t)psn3() <<  0);
        }
        l4_uint32_t myr() const { return 2000 + (mdt() >> 4); }
        l4_uint32_t mmth() const { return mdt() & 0xf; }
      } sd;
    };

  };
  static_assert(sizeof(Reg_cid) == 16, "Size of Reg_cid!");

  /**
   * 136-bit answer.
   * Sent in response to CMD9 (SEND_CSD).
   */
  struct Reg_csd
  {
    explicit Reg_csd(l4_uint32_t const r[4])
    {
      for (unsigned i = 0; i < 4; ++i)
        raw[i] = r[i];
    }

    l4_uint32_t csd_structure() const
    { return s1.csd_structure(); }

    union
    {
      struct
      { l4_uint32_t raw[4]; };

      struct // csd_structure = 3
      {
        l4_uint32_t raw0, raw1, raw2, raw3;

        CXX_BITFIELD_MEMBER_RO(30, 31, csd_structure,      raw0);
        CXX_BITFIELD_MEMBER_RO(26, 29, spec_vers,          raw0);
        CXX_BITFIELD_MEMBER_RO(16, 23, taac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 8, 15, nsac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 3,  6, tran_speed_mult,    raw0);
        CXX_BITFIELD_MEMBER_RO( 0,  2, tran_speed_unit,    raw0);

        CXX_BITFIELD_MEMBER_RO(20, 31, ccc,                raw1);
        CXX_BITFIELD_MEMBER_RO(16, 19, read_bl_len,        raw1);
        CXX_BITFIELD_MEMBER_RO(15, 15, read_bl_partial,    raw1);
        CXX_BITFIELD_MEMBER_RO(14, 14, write_blk_misalign, raw1);
        CXX_BITFIELD_MEMBER_RO(13, 13, read_blk_misalign,  raw1);
        CXX_BITFIELD_MEMBER_RO(12, 12, dsr_imp,            raw1);
        CXX_BITFIELD_MEMBER_RO( 0,  9, c_size_hi,          raw1);

        CXX_BITFIELD_MEMBER_RO(30, 31, c_size_lo,          raw2);
        CXX_BITFIELD_MEMBER_RO(27, 29, vdd_r_curr_min,     raw2);
        CXX_BITFIELD_MEMBER_RO(24, 26, vdd_r_curr_max,     raw2);
        CXX_BITFIELD_MEMBER_RO(21, 23, vdd_w_curr_min,     raw2);
        CXX_BITFIELD_MEMBER_RO(18, 20, vdd_w_curr_max,     raw2);
        CXX_BITFIELD_MEMBER_RO(15, 17, c_size_mult,        raw2);
        CXX_BITFIELD_MEMBER_RO(10, 14, erase_grp_size,     raw2);
        CXX_BITFIELD_MEMBER_RO( 5,  9, erase_grp_mult,     raw2);
        CXX_BITFIELD_MEMBER_RO( 0,  4, wp_grp_size,        raw2);

        CXX_BITFIELD_MEMBER_RO(31, 31, wp_grp_enable,      raw3);
        CXX_BITFIELD_MEMBER_RO(29, 30, default_ecc,        raw3);
        CXX_BITFIELD_MEMBER_RO(26, 28, r2w_factor,         raw3);
        CXX_BITFIELD_MEMBER_RO(22, 25, write_bl_len,       raw3);
        CXX_BITFIELD_MEMBER_RO(21, 21, write_bl_partial,   raw3);
        CXX_BITFIELD_MEMBER_RO(16, 16, content_prot_app,   raw3);
        CXX_BITFIELD_MEMBER   (15, 15, file_format_grp,    raw3);
        CXX_BITFIELD_MEMBER   (14, 14, copy,               raw3);
        CXX_BITFIELD_MEMBER   (13, 13, perm_write_protect, raw3);
        CXX_BITFIELD_MEMBER   (12, 12, tmp_write_protect,  raw3);
        CXX_BITFIELD_MEMBER   (10, 11, file_format,        raw3);
        CXX_BITFIELD_MEMBER   ( 8,  9, ecc,                raw3);
        CXX_BITFIELD_MEMBER   ( 1,  7, crc,                raw3);

        /**
         * Return the size of the device in bytes.
         *
         * \return 0 if size >= 2G. See EXT_CSD in that case.
         */
        l4_uint64_t device_size() const
        {
          l4_uint32_t c_size = ((l4_uint32_t)c_size_hi() << 2) | c_size_lo();
          if (c_size == 0xfff)
            return 0;

          l4_uint32_t mult = 1U << (c_size_mult() + 2);
          l4_uint32_t block_len = 1U << read_bl_len();
          return (c_size + 1) * mult * block_len;
        }

        l4_uint32_t tran_speed() const
        {
          l4_uint32_t freq = 100000 / 10;
          for (unsigned i = 0; i < tran_speed_unit(); ++i)
            freq *= 10;
          switch (tran_speed_mult())
            {
            case 0x1: return freq * 10;
            case 0x2: return freq * 12;
            case 0x3: return freq * 13;
            case 0x4: return freq * 15;
            case 0x5: return freq * 20;
            case 0x6: return freq * 26;
            case 0x7: return freq * 30;
            case 0x8: return freq * 35;
            case 0x9: return freq * 40;
            case 0xa: return freq * 45;
            case 0xb: return freq * 52;
            case 0xc: return freq * 55;
            case 0xd: return freq * 60;
            case 0xe: return freq * 70;
            case 0xf: return freq * 80;
            default:  return freq;
            }
        }
      } s3;

      struct // csd_structure = 1
      {
        l4_uint32_t raw0, raw1, raw2, raw3;

        CXX_BITFIELD_MEMBER_RO(30, 31, csd_structure,      raw0);
        CXX_BITFIELD_MEMBER_RO(16, 23, taac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 8, 15, nsac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 3,  6, tran_speed_mult,    raw0);
        CXX_BITFIELD_MEMBER_RO( 0,  2, tran_speed_unit,    raw0);

        CXX_BITFIELD_MEMBER_RO(20, 31, ccc,                raw1);
        CXX_BITFIELD_MEMBER_RO(16, 19, read_bl_len,        raw1);
        CXX_BITFIELD_MEMBER_RO(15, 15, read_bl_partial,    raw1);
        CXX_BITFIELD_MEMBER_RO(14, 14, write_blk_misalign, raw1);
        CXX_BITFIELD_MEMBER_RO(13, 13, read_blk_misalign,  raw1);
        CXX_BITFIELD_MEMBER_RO(12, 12, dsr_imp,            raw1);
        CXX_BITFIELD_MEMBER_RO( 0,  5, c_size_hi,          raw1);

        CXX_BITFIELD_MEMBER_RO(16, 31, c_size_lo,          raw2);
        CXX_BITFIELD_MEMBER_RO(14, 14, erase_blk_en,       raw2);
        CXX_BITFIELD_MEMBER_RO( 7, 13, sector_size,        raw2);
        CXX_BITFIELD_MEMBER_RO( 0,  6, wp_grp_size_hi,     raw2);

        CXX_BITFIELD_MEMBER_RO(31, 31, wp_grp_enable,      raw3);
        CXX_BITFIELD_MEMBER_RO(26, 28, r2w_factor,         raw3);
        CXX_BITFIELD_MEMBER_RO(22, 25, write_bl_len,       raw3);
        CXX_BITFIELD_MEMBER_RO(21, 21, write_bl_partial,   raw3);
        CXX_BITFIELD_MEMBER   (15, 15, file_format_grp,    raw3);
        CXX_BITFIELD_MEMBER   (14, 14, copy,               raw3);
        CXX_BITFIELD_MEMBER   (13, 13, perm_write_protect, raw3);
        CXX_BITFIELD_MEMBER   (12, 12, tmp_write_protect,  raw3);
        CXX_BITFIELD_MEMBER   (10, 11, file_format,        raw3);
        CXX_BITFIELD_MEMBER   ( 8,  9, ecc,                raw3);
        CXX_BITFIELD_MEMBER   ( 1,  7, crc,                raw3);

        /**
         * Return the size of the device in bytes (up to 2TB).
         */
        l4_uint64_t device_size() const
        {
          l4_uint64_t c_size = ((l4_uint64_t)c_size_hi() << 16) | c_size_lo();
          return (c_size + 1) << 19;
        }

        l4_uint32_t tran_speed() const
        {
          l4_uint32_t freq = 100000 / 10;
          for (unsigned i = 0; i < tran_speed_unit(); ++i)
            freq *= 10;
          switch (tran_speed_mult())
            {
            case 0x1: return freq * 10;
            case 0x2: return freq * 12;
            case 0x3: return freq * 13;
            case 0x4: return freq * 15;
            case 0x5: return freq * 20;
            case 0x6: return freq * 26;
            case 0x7: return freq * 30;
            case 0x8: return freq * 35;
            case 0x9: return freq * 40;
            case 0xa: return freq * 45;
            case 0xb: return freq * 52;
            case 0xc: return freq * 55;
            case 0xd: return freq * 60;
            case 0xe: return freq * 70;
            case 0xf: return freq * 80;
            default:  return freq;
            }
        }
      } s1;

      struct // csd_structure = 0
      {
        l4_uint32_t raw0, raw1, raw2, raw3;

        CXX_BITFIELD_MEMBER_RO(30, 31, csd_structure,      raw0);
        CXX_BITFIELD_MEMBER_RO(16, 23, taac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 8, 15, nsac,               raw0);
        CXX_BITFIELD_MEMBER_RO( 3,  6, tran_speed_mult,    raw0);
        CXX_BITFIELD_MEMBER_RO( 0,  2, tran_speed_unit,    raw0);

        CXX_BITFIELD_MEMBER_RO(20, 31, ccc,                raw1);
        CXX_BITFIELD_MEMBER_RO(16, 19, read_bl_len,        raw1);
        CXX_BITFIELD_MEMBER_RO(15, 15, read_bl_partial,    raw1);
        CXX_BITFIELD_MEMBER_RO(14, 14, write_blk_misalign, raw1);
        CXX_BITFIELD_MEMBER_RO(13, 13, read_blk_misalign,  raw1);
        CXX_BITFIELD_MEMBER_RO(12, 12, dsr_imp,            raw1);
        CXX_BITFIELD_MEMBER_RO( 0,  9, c_size_hi,          raw1);

        CXX_BITFIELD_MEMBER_RO(30, 31, c_size_lo,          raw2);
        CXX_BITFIELD_MEMBER_RO(27, 29, vdd_r_curr_min,     raw2);
        CXX_BITFIELD_MEMBER_RO(24, 26, vdd_r_curr_max,     raw2);
        CXX_BITFIELD_MEMBER_RO(21, 23, vdd_w_curr_min,     raw2);
        CXX_BITFIELD_MEMBER_RO(18, 20, vdd_w_curr_max,     raw2);
        CXX_BITFIELD_MEMBER_RO(15, 17, c_size_mult,        raw2);
        CXX_BITFIELD_MEMBER_RO(14, 14, erase_blk_en,       raw2);
        CXX_BITFIELD_MEMBER_RO( 7, 13, sector_size,        raw2);
        CXX_BITFIELD_MEMBER_RO( 0,  6, wp_grp_size,        raw2);

        CXX_BITFIELD_MEMBER_RO(31, 31, wp_grp_enable,      raw3);
        CXX_BITFIELD_MEMBER_RO(26, 28, r2w_factor,         raw3);
        CXX_BITFIELD_MEMBER_RO(22, 25, write_bl_len,       raw3);
        CXX_BITFIELD_MEMBER_RO(21, 21, write_bl_partial,   raw3);
        CXX_BITFIELD_MEMBER   (15, 15, file_format_grp,    raw3);
        CXX_BITFIELD_MEMBER   (14, 14, copy,               raw3);
        CXX_BITFIELD_MEMBER   (13, 13, perm_write_protect, raw3);
        CXX_BITFIELD_MEMBER   (12, 12, tmp_write_protect,  raw3);
        CXX_BITFIELD_MEMBER   (10, 11, file_format,        raw3);
        CXX_BITFIELD_MEMBER   ( 8,  9, ecc,                raw3);
        CXX_BITFIELD_MEMBER   ( 1,  7, crc,                raw3);

        /**
         * Return the size of the device in bytes.
         *
         * \return 0 if size >= 2G. See EXT_CSD in that case.
         */
        l4_uint64_t device_size() const
        {
          l4_uint32_t c_size = ((l4_uint32_t)c_size_hi() << 2) | c_size_lo();
          l4_uint32_t mult = 1U << (c_size_mult() + 2);
          l4_uint32_t block_len = 1U << read_bl_len();
          return (c_size + 1) * mult * block_len;
        }

        l4_uint32_t tran_speed() const
        {
          l4_uint32_t freq = 100000 / 10;
          for (unsigned i = 0; i < tran_speed_unit(); ++i)
            freq *= 10;
          switch (tran_speed_mult())
            {
            case 0x1: return freq * 10;
            case 0x2: return freq * 12;
            case 0x3: return freq * 13;
            case 0x4: return freq * 15;
            case 0x5: return freq * 20;
            case 0x6: return freq * 26;
            case 0x7: return freq * 30;
            case 0x8: return freq * 35;
            case 0x9: return freq * 40;
            case 0xa: return freq * 45;
            case 0xb: return freq * 52;
            case 0xc: return freq * 55;
            case 0xd: return freq * 60;
            case 0xe: return freq * 70;
            case 0xf: return freq * 80;
            default:  return freq;
            }
        }

      } s0;

    };
    void dump() const;
  };
  static_assert(sizeof(Reg_csd) == 16, "Size of Reg_csd!");

  /**
   * OCR register.
   * SD Specifications Part 1 Physical Layer Simplified Specification.
   */
  struct Reg_ocr
  {
    explicit Reg_ocr(l4_uint32_t r) : raw(r) {}
    l4_uint32_t raw;

    /// Voltage window (millivolt).

    CXX_BITFIELD_MEMBER(7, 23, voltrange_mmc, raw); ///< For MMC devices.
    CXX_BITFIELD_MEMBER(15, 23, voltrange_sd, raw); ///< For SD devices.
    CXX_BITFIELD_MEMBER(7, 7, mv1700_1950, raw);
    CXX_BITFIELD_MEMBER(8, 8, mv2000_2100, raw);
    CXX_BITFIELD_MEMBER(9, 9, mv2100_2200, raw);
    CXX_BITFIELD_MEMBER(10, 10, mv2200_2300, raw);
    CXX_BITFIELD_MEMBER(11, 11, mv2300_2400, raw);
    CXX_BITFIELD_MEMBER(12, 12, mv2400_2500, raw);
    CXX_BITFIELD_MEMBER(13, 13, mv2500_2600, raw);
    CXX_BITFIELD_MEMBER(14, 14, mv2600_2700, raw);
    CXX_BITFIELD_MEMBER(15, 15, mv2700_2800, raw);
    CXX_BITFIELD_MEMBER(16, 16, mv2800_2900, raw);
    CXX_BITFIELD_MEMBER(17, 17, mv2900_3000, raw);
    CXX_BITFIELD_MEMBER(18, 18, mv3000_3100, raw);
    CXX_BITFIELD_MEMBER(19, 19, mv3100_3200, raw);
    CXX_BITFIELD_MEMBER(20, 20, mv3200_3300, raw);
    CXX_BITFIELD_MEMBER(21, 21, mv3300_3400, raw);
    CXX_BITFIELD_MEMBER(22, 22, mv3400_3500, raw);
    CXX_BITFIELD_MEMBER(23, 23, mv3500_3600, raw);
    CXX_BITFIELD_MEMBER(24, 24, s18a, raw);        ///< SD: switch to 1.8V OK
    CXX_BITFIELD_MEMBER(27, 27, co2t, raw);        ///< SD: > 2TB (only SDUC).
    CXX_BITFIELD_MEMBER(29, 29, uhsii, raw);       ///< SD: UHS-II card status
    CXX_BITFIELD_MEMBER(30, 30, ccs, raw);         ///< SD: Card capacity status
    CXX_BITFIELD_MEMBER(31, 31, not_busy, raw);    ///< 0: Card power up busy
  };
  static_assert(sizeof(Reg_ocr) == 4, "Size of Reg_ocr!");

  /**
   * Response to CMD5: IO_SEND_OP_COND (SDIO cards only).
   */
  struct Rsp_r4
  {
    explicit Rsp_r4(l4_uint32_t r) : raw(r) {}
    l4_uint32_t raw;
    CXX_BITFIELD_MEMBER(31, 31, card_ready, raw); /// card ready after init
    CXX_BITFIELD_MEMBER(28, 30, num_io, raw);   /// number of I/O functions
    CXX_BITFIELD_MEMBER(27, 27, mem_pres, raw); /// memory present
    CXX_BITFIELD_MEMBER(24, 24, s18a, raw);     /// switch to 1.8V OK
    CXX_BITFIELD_MEMBER(0, 23, io_ocr, raw);    /// OCR
  };

  /**
   * Response to CMD8: SEND_IF_COND (SD cards only).
   */
  struct Rsp_r7
  {
    explicit Rsp_r7(l4_uint32_t r) : raw(r) {}
    l4_uint32_t raw;
    CXX_BITFIELD_MEMBER(8, 11, voltage_accepted, raw);
    CXX_BITFIELD_MEMBER(0, 7, echo_pattern, raw);
  };

  /**
   * Structure of the Extended CSD register.
   * Sent in response to CMD8 (SEND_EXT_CSD).
   */
  struct Reg_ecsd
  {
    enum Index
    {
      Reg15_cmdq_mode_en = 15,
      Reg16_secure_removal_type = 16,
      Reg17_product_state_awareness_enablement = 17,
      Reg18_max_pre_loading_data_size = 18,
      Reg22_pre_loading_data_size = 22,
      Reg26_fpu_status = 26,
      Reg29_mode_operation_codes = 29,
      Reg30_mode_config = 30,
      Reg31_barrier_ctrl = 31,
      Reg32_flush_cache = 32,
      Reg33_cache_ctrl = 33,
      Reg34_power_off_notification = 34,
      Reg35_packed_failure_index = 35,
      Reg36_packed_command_status = 36,
      Reg37_context_conf = 37,
      Reg52_ext_partitions_attribute = 52,
      Reg54_exception_events_status = 54,
      Reg56_exception_events_ctrl = 56,
      Reg58_dyncap_needed = 58,
      Reg59_class_6_ctrl = 69,
      Reg60_ini_timeout_emu = 60,
      Reg61_data_sector_size = 61,
      Reg62_use_native_sector = 62,
      Reg63_native_sector_size = 63,
      Reg64_vendor_specific_field = 64,
      Reg130_program_cid_csd_ddr_support = 130,
      Reg131_periodic_wakeup = 131,
      Reg132_tcase_support = 132,
      Reg133_production_state_awareness = 133,
      Reg134_sec_bad_blk_mgnt = 134,
      Reg136_enh_start_addr = 136,
      Reg140_enh_size_mult = 140,
      Reg143_gp_size_mult = 143,
      Reg155_partition_setting_completed = 155,
      Reg156_partitions_attribute = 156,
      Reg157_max_enh_size_mult = 157,
      Reg160_partition_support = 160,
      Reg161_hpi_mgmt = 161,
      Reg162_rst_n_function = 162,
      Reg163_bkops_en = 163,
      Reg164_bkops_start = 164,
      Reg165_sanitize_start = 165,
      Reg166_wr_rel_param = 166,
      Reg167_wr_rel_set = 167,
      Reg168_rpmb_size_mult = 168,
      Reg169_fw_config = 169,
      Reg171_user_wp = 171,
      Reg173_boot_wp = 173,
      Reg174_boot_wp_status = 174,
      Reg175_erase_group_def = 175,
      Reg177_boot_bus_conditions = 177,
      Reg178_boot_config_prot = 178,
      Reg179_partition_config = 179,
      Reg181_erased_mem_cont = 181,
      Reg183_bus_width = 183,
      Reg184_strobe_support = 184,
      Reg185_hs_timing = 185,
      Reg187_power_class = 187,
      Reg189_cmd_set_rev = 189,
      Reg191_cmd_set = 191,
      Reg192_ext_csd_rev = 192,
      Reg194_csd_structure = 194,
      Reg196_device_type = 196,
      Reg197_driver_strength = 197,
      Reg198_out_of_interrupt_time = 198,
      Reg199_partition_switch_time = 199,
      Reg200_pwr_cl_52_195 = 200,
      Reg201_pwr_cl_26_195 = 201,
      Reg202_pwr_cl_52_360 = 202,
      Reg203_pwr_cl_26_360 = 203,
      Reg205_min_perf_r_4_26 = 205,
      Reg206_min_perf_w_4_26 = 206,
      Reg207_min_perf_r_8_26_4_52 = 207,
      Reg208_min_perf_w_8_26_4_52 = 208,
      Reg209_min_perf_r_8_52 = 209,
      Reg210_min_perf_w_8_52 = 210,
      Reg211_secure_wp_info = 211,
      Reg212_sec_count = 212,
      Reg216_sleep_notification_time = 216,
      Reg217_s_a_timeout = 217,
      Reg218_production_state_awareness_timeout = 218,
      Reg219_s_c_vccq = 219,
      Reg220_s_c_vcc = 220,
      Reg221_hc_wp_grp_size = 221,
      Reg222_rel_wr_sec_c = 222,
      Reg223_erase_timeout_mult = 223,
      Reg224_hc_erase_grp_size = 224,
      Reg225_acc_size = 225,
      Reg226_boot_size_mult = 226,
      Reg228_boot_info = 228,
      Reg229_sec_trim_mult = 229,
      Reg230_sec_erase_mult = 230,
      Reg231_sec_feature_support = 231,
      Reg232_trim_mult = 232,
      Reg234_min_perf_ddr_r_8_52 = 234,
      Reg235_min_perf_ddr_w_8_52 = 235,
      Reg236_pwr_cl_200_130 = 236,
      Reg237_pwr_cl_200_195 = 237,
      Reg238_pwr_cl_ddr_52_195 = 238,
      Reg239_pwr_cl_ddr_52_360 = 239,
      Reg240_cache_flush_policy = 240,
      Reg241_ini_timeout_ap = 241,
      Reg242_correctly_prg_sectors_num = 242,
      Reg246_bkops_status = 246,
      Reg247_power_off_long_time = 247,
      Reg248_generic_cmd6_time = 248,
      Reg249_cache_size = 249,
      Reg253_pwr_cl_ddr_200_360 = 253,
      Reg254_firmware_version = 254,
      Reg262_device_version = 262,
      Reg264_optimal_trim_unit_size = 264,
      Reg265_optimal_write_size = 265,
      Reg266_optimal_read_size = 266,
      Reg267_pre_eol_info = 267,
      Reg268_device_life_time_est_typ_a = 268,
      Reg269_device_life_time_est_typ_b = 269,
      Reg270_vendor_proprietary_health_report = 270,
      Reg302_number_of_fw_sectors_correctly_programmed = 302,
      Reg307_cmdq_depth = 307,
      Reg308_cmdq_support = 308,
      Reg486_barrier_support = 486,
      Reg487_fpu_arg = 487,
      Reg491_operation_code_timeout = 491,
      Reg492_ffu_features = 492,
      Reg493_supported_modes = 493,
      Reg494_ext_support = 494,
      Reg495_larg_unit_size_m1 = 495,
      Reg496_context_capabilities = 496,
      Reg497_tag_res_size = 497,
      Reg498_tag_unit_size = 498,
      Reg499_data_tag_support = 499,
      Reg500_max_packed_writes = 500,
      Reg501_max_packet_reads = 501,
      Reg502_bkops_support = 502,
      Reg503_hpi_features = 503,
      Reg504_s_cmd_set = 504,
      Reg505_ext_security_err = 505,
    };

    template <enum Index reg_index>
    struct Reg8
    {
      l4_uint8_t raw;
      explicit Reg8() {}
      explicit Reg8(l4_uint8_t v) : raw(v) {}
      Index index() const { return reg_index; }
    };

    l4_uint8_t ec0_reserved[15];
    l4_uint8_t ec15_cmdq_mode_en;
    l4_uint8_t ec16_secure_removal_type;
    l4_uint8_t ec17_product_state_awareness_enablement;
    l4_uint8_t ec18_max_pre_loading_data_size[4];
    l4_uint8_t ec22_pre_loading_data_size[4];
    l4_uint8_t ec26_fpu_status;
    l4_uint8_t ec27_reserved[2];
    l4_uint8_t ec29_mode_operation_codes;
    l4_uint8_t ec30_mode_config;
    l4_uint8_t ec31_barrier_ctrl;
    struct Ec32_flush_cache : public Reg8<Reg32_flush_cache>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 0, flush, raw);
      CXX_BITFIELD_MEMBER(1, 1, barrier, raw);
    };
    Ec32_flush_cache ec32_flush_cache;
    struct Ec33_cache_ctrl : public Reg8<Reg33_cache_ctrl>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 0, cache_en, raw);
    };
    Ec33_cache_ctrl ec33_cache_ctrl;
    struct Ec34_power_off_notification : public Reg8<Reg34_power_off_notification>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 7, notify, raw);
      enum
      {
        No_power_notification = 0,
        Powered_on = 1,
        Power_off_short = 2,
        Power_off_long = 3,
        Sleep_notification = 4,
      };
    };
    Ec34_power_off_notification ec34_power_off_notification;
    l4_uint8_t ec35_packed_failure_index;
    l4_uint8_t ec36_packed_command_status;
    l4_uint8_t ec37_context_conf[15];
    l4_uint8_t ec52_ext_partitions_attribute[2];
    l4_uint8_t ec54_exception_events_status[2];
    l4_uint8_t ec56_exception_events_ctrl[2];
    l4_uint8_t ec58_dyncap_needed;
    l4_uint8_t ec59_class_6_ctrl;
    l4_uint8_t ec60_ini_timeout_emu;
    l4_uint8_t ec61_data_sector_size;
    l4_uint8_t ec62_use_native_sector;
    l4_uint8_t ec63_native_sector_size;
    l4_uint8_t ec64_vendor_specific_field[64];
    l4_uint8_t ec128_reserved[2];
    l4_uint8_t ec130_program_cid_csd_ddr_support;
    l4_uint8_t ec131_periodic_wakeup;
    l4_uint8_t ec132_tcase_support;
    l4_uint8_t ec133_production_state_awareness;
    l4_uint8_t ec134_sec_bad_blk_mgnt;
    l4_uint8_t ec135_reserved;
    l4_uint8_t ec136_enh_start_addr[4];
    l4_uint8_t ec140_enh_size_mult[3];
    l4_uint8_t ec143_gp_size_mult[12];
    l4_uint8_t ec155_partition_setting_completed;
    l4_uint8_t ec156_partitions_attribute;
    l4_uint8_t ec157_max_enh_size_mult[3];
    l4_uint8_t ec160_partition_support;
    struct Ec161_hpi_mgmt : public Reg8<Reg161_hpi_mgmt>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 0, hpi_en, raw);
    };
    Ec161_hpi_mgmt ec161_hpi_mgmt;
    l4_uint8_t ec162_rst_n_function;
    struct Ec163_bkops_en : public Reg8<Reg163_bkops_en>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(1, 1, auto_en, raw);
      CXX_BITFIELD_MEMBER(0, 0, manual_en, raw);
    };
    l4_uint8_t ec163_bkops_en;
    l4_uint8_t ec164_bkops_start;
    l4_uint8_t ec165_sanitize_start;
    l4_uint8_t ec166_wr_rel_param;
    l4_uint8_t ec167_wr_rel_set;
    l4_uint8_t ec168_rpmb_size_mult;
    l4_uint8_t ec169_fw_config;
    l4_uint8_t ec170_reserved;
    l4_uint8_t ec171_user_wp;
    l4_uint8_t ec172_reserved;
    l4_uint8_t ec173_boot_wp;
    l4_uint8_t ec174_boot_wp_status;
    struct Ec175_erase_group_def : public Reg8<Reg175_erase_group_def>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 0, enable, raw);
    };
    Ec175_erase_group_def ec175_erase_group_def;
    l4_uint8_t ec176_reserved;
    l4_uint8_t ec177_boot_bus_conditions;
    l4_uint8_t ec178_boot_config_prot;
    struct Ec179_partition_config : public Reg8<Reg179_partition_config>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(6, 6, boot_ack, raw);
      CXX_BITFIELD_MEMBER(3, 5, boot_part_enable, raw);
      CXX_BITFIELD_MEMBER(0, 2, partition_access, raw);
      char const *str_partition_access() const
      {
        switch (partition_access())
          {
          case 0: return "user";
          case 1: return "boot1";
          case 2: return "boot2";
          case 3: return "RPMB";
          case 4: return "general purpose partition 1";
          case 5: return "general purpose partition 2";
          case 6: return "general purpose partition 3";
          case 7: return "general purpose partition 4";
          default: return "unknown";
          }
      }
    };
    Ec179_partition_config ec179_partition_config;
    l4_uint8_t ec180_reserved;
    l4_uint8_t ec181_erased_mem_cont;
    l4_uint8_t ec182_reserved;
    struct Ec183_bus_width : public Reg8<Reg183_bus_width>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(7, 7, enhanced_strobe, raw);
      CXX_BITFIELD_MEMBER(0, 3, bus_mode_select, raw);
      enum Width
      {
        W_1bit_sdr = 0,
        W_4bit_sdr = 1,
        W_8bit_sdr = 2,
        W_4bit_ddr = 5,
        W_8bit_ddr = 6,
      };
      char const *str_bus_width() const
      {
        switch (bus_mode_select())
          {
          case W_1bit_sdr: return "1-bit data bus";
          case W_4bit_sdr: return "4-bit data bus";
          case W_8bit_sdr: return "8-bit data bus";
          case W_4bit_ddr: return "4-bit data bus (DDR)";
          case W_8bit_ddr: return "8-bit data bus (DDR)";
          default:         return "unknown";
          }
      }
    };
    Ec183_bus_width ec183_bus_width;
    l4_uint8_t ec184_strobe_support;
    struct Ec185_hs_timing : public Reg8<Reg185_hs_timing>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(4, 7, driver_strength, raw);
      CXX_BITFIELD_MEMBER(0, 3, timing_interface, raw);
      enum Timing
      {
        Backward_compat = 0,
        Hs = 1,
        Hs200 = 2,
        Hs400 = 3,
      };
      char const *str_timing_interface() const
      {
        switch (timing_interface())
          {
          case Backward_compat: return "backward-compatible";
          case Hs:              return "High-Speed";
          case Hs200:           return "HS200";
          case Hs400:           return "HS400";
          default:              return "unknown";
          }
      }
      CXX_BITFIELD_MEMBER(4, 7, selected_strength, raw);
      enum
      {
        Type_0 = 0,
        Type_1 = 1,
        Type_2 = 2,
        Type_3 = 3,
        Type_4 = 4,
      };
    };
    Ec185_hs_timing ec185_hs_timing;
    l4_uint8_t ec186_reserved;
    l4_uint8_t ec187_power_class;
    l4_uint8_t ec188_reserved;
    l4_uint8_t ec189_cmd_set_rev;
    l4_uint8_t ec190_reserved;
    l4_uint8_t ec191_cmd_set;
    struct Ec192_ext_csd_rev : public Reg8<Reg192_ext_csd_rev>
    {
      using Reg8::Reg8;
      l4_uint32_t csd_rev() const { return raw; }
      l4_uint32_t mmc_rev() const
      {
        switch (raw)
          {
          case 0:
          case 1:
          case 2:
          case 3:
          case 4: return raw + 400;
          case 5: return 441;
          case 6: return 451;
          case 7: return 500;
          case 8: return 510;
          default: return 0;
          }
      }
    };
    Ec192_ext_csd_rev ec192_ext_csd_rev;

    // <<< NOT WRITABLE AFTER THIS POINT! >>>
    l4_uint8_t ec193_reserved;
    l4_uint8_t ec194_csd_structure;
    l4_uint8_t ec195_reserved;
    struct Ec196_device_type : public Reg8<Reg196_device_type>
    {
      using Reg8::Reg8;
      enum Bit
      {
        Bt_hs400_ddr_12 = 7,
        Bt_hs400_ddr_18 = 6,
        Bt_hs200_sdr_12 = 5,
        Bt_hs200_sdr_18 = 4,
        Bt_hs52_ddr_12 = 3,
        Bt_hs52_ddr_18 = 2,
        Bt_hs52 = 1,
        Bt_hs26 = 0,
      };
      CXX_BITFIELD_MEMBER(Bt_hs400_ddr_12, Bt_hs400_ddr_12, hs400_ddr_12, raw);
      CXX_BITFIELD_MEMBER(Bt_hs400_ddr_18, Bt_hs400_ddr_18, hs400_ddr_18, raw);
      CXX_BITFIELD_MEMBER(Bt_hs200_sdr_12, Bt_hs200_sdr_12, hs200_sdr_12, raw);
      CXX_BITFIELD_MEMBER(Bt_hs200_sdr_18, Bt_hs200_sdr_18, hs200_sdr_18, raw);
      CXX_BITFIELD_MEMBER(Bt_hs52_ddr_12, Bt_hs52_ddr_12, hs52_ddr_12, raw);
      CXX_BITFIELD_MEMBER(Bt_hs52_ddr_18, Bt_hs52_ddr_18, hs52_ddr_18, raw);
      CXX_BITFIELD_MEMBER(Bt_hs52, Bt_hs52, hs52, raw);
      CXX_BITFIELD_MEMBER(Bt_hs26, Bt_hs26, hs26, raw);
      static char const *str_device_type(unsigned type)
      {
        switch (type)
          {
          case 1 << Bt_hs26: return "High-Speed eMMC at 26MHz";
          case 1 << Bt_hs52: return "High-Speed eMMC at 52MHz";
          case 1 << Bt_hs52_ddr_18:
            return "High-Speed DDR eMMC at 52MHz (1.8V or 3V)";
          case 1 << Bt_hs52_ddr_12:
            return "High-Speed DDR eMMC at 52MHz (1.2V)";
          case 1 << Bt_hs200_sdr_18:
            return "HS200 Single Data Rate eMMC at 200MHz (1.8V)";
          case 1 << Bt_hs200_sdr_12:
            return "HS200 Single Data Rate eMMC at 200MHz (1.2V)";
          case 1 << Bt_hs400_ddr_18:
            return "HS400 Dual Data Rate eMMC at 200MHz (1.8V)";
          case 1 << Bt_hs400_ddr_12:
            return "HS400 Dual Data Rate eMMC at 200MHz (1.2V)";
          case 0:  return "Fallback";
          default: return "unknown";
          }
      }

      void disable(Ec196_device_type const &other)
      { raw &= ~other.raw; }

      void disable_12()
      {
        hs52_ddr_12() = 0;
        hs200_sdr_12() = 0;
        hs400_ddr_12() = 0;
      }

      static Ec196_device_type fallback()
      { return Ec196_device_type(0); }
    };
    Ec196_device_type ec196_device_type;
    struct Ec197_driver_strength : public Reg8<Reg197_driver_strength>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(4, 4, type4, raw); // optional
      CXX_BITFIELD_MEMBER(3, 3, type3, raw); // optional
      CXX_BITFIELD_MEMBER(2, 2, type2, raw); // optional
      CXX_BITFIELD_MEMBER(1, 1, type1, raw); // optional
      CXX_BITFIELD_MEMBER(0, 0, type0, raw); // mandatory
    };
    Ec197_driver_strength ec197_driver_strength;
    l4_uint8_t ec198_out_of_interrupt_time;
    l4_uint8_t ec199_partition_switch_time;
    l4_uint8_t ec200_pwr_cl_52_195;
    l4_uint8_t ec201_pwr_cl_26_195;
    l4_uint8_t ec202_pwr_cl_52_360;
    l4_uint8_t ec203_pwr_cl_26_360;
    l4_uint8_t ec204_reserved5;
    l4_uint8_t ec205_min_perf_r_4_26;
    l4_uint8_t ec206_min_perf_w_4_26;
    l4_uint8_t ec207_min_perf_r_8_26_4_52;
    l4_uint8_t ec208_min_perf_w_8_26_4_52;
    l4_uint8_t ec209_min_perf_r_8_52;
    l4_uint8_t ec210_min_perf_w_8_52;
    l4_uint8_t ec211_secure_wp_info;
    l4_uint32_t ec212_sec_count;
    l4_uint8_t ec216_sleep_notification_time;
    l4_uint8_t ec217_s_a_timeout;
    l4_uint8_t ec218_production_state_awareness_timeout;
    l4_uint8_t ec219_s_c_vccq;
    l4_uint8_t ec220_s_c_vcc;
    l4_uint8_t ec221_hc_wp_grp_size;
    l4_uint8_t ec222_rel_wr_sec_c;
    l4_uint8_t ec223_erase_timeout_mult;
    l4_uint8_t ec224_hc_erase_grp_size;
    l4_uint8_t ec225_acc_size;
    l4_uint8_t ec226_boot_size_mult;
    l4_uint8_t ec227_reserved;
    l4_uint8_t ec228_boot_info;
    l4_uint8_t ec229_sec_trim_mult;
    l4_uint8_t ec230_sec_erase_mult;
    l4_uint8_t ec231_sec_feature_support;
    l4_uint8_t ec232_trim_mult;
    l4_uint8_t ec233_reserved;
    l4_uint8_t ec234_min_perf_ddr_r_8_52;
    l4_uint8_t ec235_min_perf_ddr_w_8_52;
    l4_uint8_t ec236_pwr_cl_200_130;
    l4_uint8_t ec237_pwr_cl_200_195;
    l4_uint8_t ec238_pwr_cl_ddr_52_195;
    l4_uint8_t ec239_pwr_cl_ddr_52_360;
    l4_uint8_t ec240_cache_flush_policy;
    l4_uint8_t ec241_ini_timeout_ap;
    l4_uint8_t ec242_correctly_prg_sectors_num[4];
    l4_uint8_t ec246_bkops_status;
    l4_uint8_t ec247_power_off_long_time;
    l4_uint8_t ec248_generic_cmd6_time;
    l4_uint8_t ec249_cache_size[4];
    l4_uint8_t ec253_pwr_cl_ddr_200_360;
    l4_uint8_t ec254_firmware_version[8];
    l4_uint8_t ec262_device_version[2];
    l4_uint8_t ec264_optimal_trim_unit_size;
    l4_uint8_t ec265_optimal_write_size;
    l4_uint8_t ec266_optimal_read_size;
    l4_uint8_t ec267_pre_eol_info;
    l4_uint8_t ec268_device_life_time_est_typ_a;
    l4_uint8_t ec269_device_life_time_est_typ_b;
    static std::string lifetime_est(l4_uint8_t t)
    {
      if (t == 0)
        return "undefined";
      if (t < 0xb)
        return std::to_string(t * 10) + "% time used";
      if (t == 0xb)
        return "exceeded";
      return "unknown";
    }
    l4_uint8_t ec270_vendor_proprietary_health_report[32];
    l4_uint8_t ec302_number_of_fw_sectors_correctly_programmed[4];
    l4_uint8_t ec306_reserved;
    l4_uint8_t ec307_cmdq_depth;
    struct Ec308_cmdq_support : public Reg8<Reg308_cmdq_support>
    {
      using Reg8::Reg8;
      CXX_BITFIELD_MEMBER(0, 0, cmdq_support, raw);
    };
    Ec308_cmdq_support ec308_cmdq_support;
    l4_uint8_t ec309_reserved[177];
    l4_uint8_t ec486_barrier_support;
    l4_uint8_t ec487_fpu_arg[4];
    l4_uint8_t ec491_operation_code_timeout;
    l4_uint8_t ec492_ffu_features;
    l4_uint8_t ec493_supported_modes;
    l4_uint8_t ec494_ext_support;
    l4_uint8_t ec495_larg_unit_size_m1;
    l4_uint8_t ec496_context_capabilities;
    l4_uint8_t ec497_tag_res_size;
    l4_uint8_t ec498_tag_unit_size;
    l4_uint8_t ec499_data_tag_support;
    l4_uint8_t ec500_max_packed_writes;
    l4_uint8_t ec501_max_packet_reads;
    l4_uint8_t ec502_bkops_support;
    l4_uint8_t ec503_hpi_features;
    l4_uint8_t ec504_s_cmd_set;
    l4_uint8_t ec505_ext_security_err;
    l4_uint8_t ec506_reserved[6];

    void dump() const;
  };
  static_assert(sizeof(Reg_ecsd) == 512, "Size of Reg_ecsd!");

  /**
   * SD Specifications Part 1 (Physical Layer Simplified Specification).
   */
  struct Reg_scr
  {
    explicit Reg_scr(l4_uint8_t const *bytes)
    {
      for (unsigned i = 0; i < sizeof(*this); ++i)
        reinterpret_cast<l4_uint8_t*>(&raw)[sizeof(*this) - 1 - i] = bytes[i];
    }
    l4_uint64_t raw;
    CXX_BITFIELD_MEMBER(60, 63, scr_structure, raw);
    CXX_BITFIELD_MEMBER(56, 59, sd_spec, raw);
    CXX_BITFIELD_MEMBER(55, 55, data_stat_after_erase, raw);
    CXX_BITFIELD_MEMBER(52, 54, sd_security, raw);
    CXX_BITFIELD_MEMBER(50, 50, sd_bus_width_4, raw);
    CXX_BITFIELD_MEMBER(48, 48, sd_bus_width_1, raw);
    CXX_BITFIELD_MEMBER(47, 47, sd_spec3, raw);
    CXX_BITFIELD_MEMBER(43, 46, ex_security, raw);
    CXX_BITFIELD_MEMBER(42, 42, sd_spec4, raw);
    CXX_BITFIELD_MEMBER(38, 41, sd_specx, raw);
    CXX_BITFIELD_MEMBER(35, 35, cmd58_cmd59_support, raw);
    CXX_BITFIELD_MEMBER(34, 34, cmd48_cmd49_support, raw);
    CXX_BITFIELD_MEMBER(33, 33, cmd23_support, raw);
    CXX_BITFIELD_MEMBER(32, 32, cmd20_support, raw);

    l4_uint32_t sd_spec_vers() const
    {
      unsigned vers = ((unsigned)sd_spec() << 12)
                    | ((unsigned)sd_spec3() << 8)
                    | ((unsigned)sd_spec4() << 4)
                    | sd_specx();
      switch (vers)
        {
        case 0x0000: return 100;
        case 0x1000: return 110;
        case 0x2000: return 200;
        case 0x2100: return 300;
        case 0x2110: return 400;
        case 0x2101:
        case 0x2111: return 500;
        case 0x2102:
        case 0x2112: return 600;
        default:     return 0;
        }
    }
    char const *sd_spec_str() const
    {
      switch (sd_spec_vers())
        {
        case 100: return "1.0x";
        case 110: return "1.10";
        case 200: return "2.00";
        case 300: return "3.0x";
        case 400: return "4.xx";
        case 500: return "5.xx";
        case 600: return "6.xx";
        default:  return "unknown";
        }
    }
  };
  static_assert(sizeof(Reg_scr) == 8, "Size of Reg_scr!");

  /**
   * SD Specifications Part 1 (Physical Layer Simplified Specification).
   */
  struct Reg_ssr
  {
    Reg_ssr() = delete;
    Reg_ssr(l4_uint8_t const *bytes)
    {
      for (unsigned i = 0; i < sizeof(*this); ++i)
        reinterpret_cast<l4_uint8_t*>(&raw0)[sizeof(*this) - 1 - i] = bytes[i];
    }
    l4_uint32_t raw0, raw1, raw2, raw3, raw4, raw5, raw6, raw7;
    l4_uint32_t raw8, raw9, raw10, raw11, raw12, raw13, raw14, raw15;
    CXX_BITFIELD_MEMBER(30, 31, dat_bus_width, raw15);
    CXX_BITFIELD_MEMBER(29, 29, secured_mode, raw15);
    CXX_BITFIELD_MEMBER( 0, 15, sd_card_type, raw15);
    CXX_BITFIELD_MEMBER( 0, 31, size_of_protected_area, raw14);
    CXX_BITFIELD_MEMBER(24, 31, speed_class, raw13);
    enum Speed_class
    {
      Class0 = 0,
      Class2 = 1,
      Class4 = 2,
      Class6 = 3,
      Class10 = 4,
    };
    char const *str_speed_class() const
    {
      switch (speed_class())
        {
        case Class0:  return "Class 0";
        case Class2:  return "Class 2";
        case Class4:  return "Class 4";
        case Class6:  return "Class 6";
        case Class10: return "Class 10";
        default:      return "unknown";
        }
    }
    CXX_BITFIELD_MEMBER(16, 23, performance_move, raw13);
    CXX_BITFIELD_MEMBER(12, 15, au_size, raw13);
    l4_uint32_t au_size_val() const
    {
      if (au_size() == 0x0)
        return 0;
      if (au_size() < 0xb)
        return 1U << (1 + au_size());
      switch (au_size())
        {
        case 0xb: return 12U << 20;
        case 0xc: return 16U << 20;
        case 0xd: return 24U << 20;
        case 0xe: return 32U << 20;
        case 0xf:
        default:  return 64U << 20;
        }
    }
    CXX_BITFIELD_MEMBER( 0,  7, erase_size_hi, raw13);
    CXX_BITFIELD_MEMBER(24, 31, erase_size_lo, raw12);
    CXX_BITFIELD_MEMBER(18, 23, erase_timeout, raw12);
    CXX_BITFIELD_MEMBER(16, 17, erase_offset, raw12);
    CXX_BITFIELD_MEMBER(12, 15, uhs_speed_grade, raw12);
    enum Uhs_speed_grade
    {
      Less_than_10mbs = 0,
      Equal_greater_10mbs = 1,
      Equal_greater_30mbs = 3,
    };
    char const *str_uhs_speed_grade() const
    {
      switch (uhs_speed_grade())
        {
        case Less_than_10mbs:     return "< 10MB/s";
        case Equal_greater_10mbs: return ">= 10MB/s";
        case Equal_greater_30mbs: return ">= 30MB/s";
        default:                  return "unknown";
        }
    }
    CXX_BITFIELD_MEMBER( 8, 11, uhs_au_size, raw12);
    l4_uint32_t uhs_au_size_val() const
    {
      if (au_size() < 0x7)
        return 0;
      if (au_size() < 0xb)
        return 1U << (1 + au_size());
      switch (au_size())
        {
        case 0xb: return 12U << 20;
        case 0xc: return 16U << 20;
        case 0xd: return 24U << 20;
        case 0xe: return 32U << 20;
        case 0xf:
        default:  return 64U << 20;
        }
    }
    CXX_BITFIELD_MEMBER( 0,  7, video_speed_class, raw12);

    CXX_BITFIELD_MEMBER(16, 25, vsc_au_size, raw11);            // 377:368

    CXX_BITFIELD_MEMBER( 9,  9, card_maint, raw10);             // 328:328
    CXX_BITFIELD_MEMBER( 9,  9, host_maint, raw10);             // 329:329
    CXX_BITFIELD_MEMBER(10, 10, supp_cache, raw10);             // 330:330
    CXX_BITFIELD_MEMBER(11, 15, supp_cmd_queue, raw10);         // 335:331
    CXX_BITFIELD_MEMBER(16, 19, app_perf_class, raw10);         // 336:339

    CXX_BITFIELD_MEMBER(24, 24, fule_support, raw9);            // 312:312
    CXX_BITFIELD_MEMBER(25, 25, discard_support, raw9);         // 313:313
  };
  static_assert(sizeof(Reg_ssr) == 64, "Size of Reg_ssr!");

  /**
   * SD Specifications Part 1 (Physical Layer Simplified Specification).
   */
  struct Reg_switch_func
  {
    Reg_switch_func() = delete;
    Reg_switch_func(l4_uint8_t const *bytes)
    {
      for (unsigned i = 0; i < sizeof(*this); ++i)
        reinterpret_cast<l4_uint8_t*>(&raw)[sizeof(*this) - 1 - i] = bytes[i];
    }
    l4_uint32_t raw[8]; // these bits are empty
    l4_uint32_t raw8, raw9, raw10, raw11, raw12, raw13, raw14, raw15;

    // Maximum Current / Power Consumption. 1 = 1mA, 2 = 2mA, ...
    /* 496 */ CXX_BITFIELD_MEMBER(16, 31, max_curr_power, raw15);

    // Support bits of functions in Group 6:
    /* 480 */ CXX_BITFIELD_MEMBER( 0, 15, supp_bits_grp6, raw15);

    // Support bits of functions in Group 5:
    /* 464 */ CXX_BITFIELD_MEMBER(16, 31, supp_bits_grp5, raw14);

    // Support bits of functions in Group 4: Power limit
    /* 448 */ CXX_BITFIELD_MEMBER( 0, 15, supp_bits_grp4, raw14);
    /* 448 */  CXX_BITFIELD_MEMBER( 4,  4, power_limit_180w, raw14);
    /* 448 */  CXX_BITFIELD_MEMBER( 3,  3, power_limit_288w, raw14);
    /* 448 */  CXX_BITFIELD_MEMBER( 2,  2, power_limit_216w, raw14);
    /* 448 */  CXX_BITFIELD_MEMBER( 1,  1, power_limit_144w, raw14);
    /* 448 */  CXX_BITFIELD_MEMBER( 0,  0, power_limit_072w, raw14);

    // Support bits of functions in Group 3: Driver strength
    /* 432 */ CXX_BITFIELD_MEMBER(16, 31, supp_bits_grp3, raw13);
    /* 435 */  CXX_BITFIELD_MEMBER(19, 19, strength_type_d, raw13);
    /* 434 */  CXX_BITFIELD_MEMBER(18, 18, strength_type_c, raw13);
    /* 433 */  CXX_BITFIELD_MEMBER(17, 17, strength_type_a, raw13);
    /* 432 */  CXX_BITFIELD_MEMBER(16, 16, strength_type_b, raw13);

    // Support bits of functions in Group 2: Command system
    /* 416 */ CXX_BITFIELD_MEMBER( 0, 15, supp_bits_grp2, raw13);

    // Support bits of functions in Group 1: Access mode / Bus speed
    /* 400 */ CXX_BITFIELD_MEMBER(16, 31, supp_bits_grp1, raw12);
    /* 404 */  CXX_BITFIELD_MEMBER(20, 20, acc_mode_ddr50, raw12);
    /* 403 */  CXX_BITFIELD_MEMBER(19, 19, acc_mode_sdr104, raw12);
    /* 402 */  CXX_BITFIELD_MEMBER(18, 18, acc_mode_sdr50, raw12);
    /* 401 */  CXX_BITFIELD_MEMBER(17, 17, acc_mode_sdr25, raw12);
    /* 400 */  CXX_BITFIELD_MEMBER(16, 16, acc_mode_sdr12, raw12);

    // Function selection of function Group 6
    /* 396 */ CXX_BITFIELD_MEMBER(12, 15, fun_sel_grp6, raw12);
    // Function selection of function Group 5
    /* 392 */ CXX_BITFIELD_MEMBER( 8, 11, fun_sel_grp5, raw12);
    // Function selection of function Group 4
    /* 388 */ CXX_BITFIELD_MEMBER( 4,  7, fun_sel_grp4, raw12);
    // Function selection of function Group 3
    /* 384 */ CXX_BITFIELD_MEMBER( 0,  3, fun_sel_grp3, raw12);
    // Function selection of function Group 2
    /* 380 */ CXX_BITFIELD_MEMBER(28, 31, fun_sel_grp2, raw11);
    // Function selection of function Group 1
    /* 376 */ CXX_BITFIELD_MEMBER(24, 27, fun_sel_grp1, raw11);

    // Data structure version
    /* 368 */ CXX_BITFIELD_MEMBER(16, 23, data_struct_vers, raw11);

    // Reserved for busy status of functions in Group 6 .. Group 1
    /* 352 */ CXX_BITFIELD_MEMBER( 0, 15, busy_stat_fun_grp6, raw11);
    /* 336 */ CXX_BITFIELD_MEMBER(16, 31, busy_stat_fun_grp5, raw10);
    /* 320 */ CXX_BITFIELD_MEMBER( 0, 15, busy_stat_fun_grp4, raw10);
    /* 304 */ CXX_BITFIELD_MEMBER(16, 31, busy_stat_fun_grp3, raw9);
    /* 288 */ CXX_BITFIELD_MEMBER( 0, 15, busy_stat_fun_grp2, raw9);
    /* 272 */ CXX_BITFIELD_MEMBER(16, 31, busy_stat_fun_grp1, raw8);

    enum { Invalid_function = 0xf };
  };
  static_assert(sizeof(Reg_switch_func) == 64, "Size of Reg_switch_func!");

  struct Arg
  {
    explicit Arg() : raw(0) {}
    explicit Arg(l4_uint32_t v) : raw(v) {}
    l4_uint32_t raw;
  };

  // eMMC spec: 6.6.21
  struct Arg_cmd5_sleep_aware : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(15, 15, sleep_awake, raw);
    enum Sleep_awake
    {
      Sleep = 1,
      Aware = 0,
    };
  };

  struct Arg_cmd6_switch : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(24, 25, access, raw);
    enum Access
    {
      Command_set = 0,
      Set_bits = 1,
      Clear_bits = 2,
      Write_byte = 3,
    };
    CXX_BITFIELD_MEMBER(16, 23, index, raw);
    CXX_BITFIELD_MEMBER(8, 15, value, raw);
    CXX_BITFIELD_MEMBER(0, 2, cmdset, raw);
  };

  struct Arg_cmd6_switch_func : public Arg
  {
    explicit Arg_cmd6_switch_func()
    { reset(); }

    void reset()
    {
      raw = 0;
      grp1_acc_mode() = Dont_change;
      grp2_cmd_system() = Dont_change;
      grp3_drive_strength() = Dont_change;
      grp4_power_limit() = Dont_change;
      grp5() = Dont_change;
      grp6() = Dont_change;
      mode() = Check_function;
    }

    CXX_BITFIELD_MEMBER(31, 31, mode, raw);
    enum Mode
    {
      Check_function = 0,
      Set_function = 1,
    };

    // bits 24-30 reserved all 0

    enum { Dont_change = 0xf };

    CXX_BITFIELD_MEMBER(20, 23, grp6, raw);
    CXX_BITFIELD_MEMBER(16, 19, grp5, raw);
    CXX_BITFIELD_MEMBER(12, 15, grp4_power_limit, raw);
    enum Grp4_power_limit
    {
      Grp4_default = 0x0,
      Grp4_072w = 0x0, // current limit 200mA at 3.6V
      Grp4_144w = 0x1, // current limit 400mA
      Grp4_216w = 0x2, // current limit 600mA (only embedded)
      Grp4_288w = 0x3, // current limit 800mA (only embedded)
      Grp4_180w = 0x4, // current limit 400mA (max allowed for removable cards)
    };
    CXX_BITFIELD_MEMBER(8, 11, grp3_drive_strength, raw);
    enum Grp3_driver_strength
    {
      Grp3_default = 0x0,
      Grp3_type_b = 0x0,
      Grp3_type_a = 0x1,
      Grp3_type_c = 0x2,
      Grp3_type_d = 0x3,
    };
    CXX_BITFIELD_MEMBER(4, 7, grp2_cmd_system, raw);
    enum Grp2_command_system
    {
      Grp2_default = 0x0,
      Grp2_for_ec = 0x1,
      Grp2_otp = 0x3,
      Grp2_assd = 0x4,
    };
    CXX_BITFIELD_MEMBER(0, 3, grp1_acc_mode, raw);
    enum Grp1_acc_mode
    {
      Grp1_sdr12 = 0x0,
      Grp1_sdr25 = 0x1,
      Grp1_sdr50 = 0x2,
      Grp1_sdr104 = 0x3,
      Grp1_ddr50 = 0x4,
    };
  };

  struct Arg_cmd8_send_if_cond : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(0, 7, check_pattern, raw);
    CXX_BITFIELD_MEMBER(8, 11, voltage_suppl, raw);
    enum Voltage_suppl
    {
      Not_defined = 0x0,
      Volt_27_36 = 0x1,   // 2.7-3.6V
      Volt_low = 0x02,    // reserved for low voltage range
    };
    CXX_BITFIELD_MEMBER(12, 12, pcie_avail, raw);
    CXX_BITFIELD_MEMBER(13, 13, pcie_12v_supp, raw);
  };

  struct Arg_cmd19_send_tuning_block : public Arg
  {
    using Arg::Arg;
    // SD Specification Part 1 (Physical Layer Simplified Specification) 4.2.4.5
    enum { Max_loops = 40 };
  };

  struct Arg_cmd21_send_tuning_block : public Arg
  {
    using Arg::Arg;
    // eMMC 6.6.5.1
    enum { Max_loops = 40 };
  };

  struct Arg_cmd23_set_block_count : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(0, 15, blocks, raw);
    CXX_BITFIELD_MEMBER(24, 24, forced_prg, raw);
    CXX_BITFIELD_MEMBER(25, 28, context_id, raw);
    CXX_BITFIELD_MEMBER(29, 29, tag_request, raw);
    CXX_BITFIELD_MEMBER(30, 30, packed, raw);
    CXX_BITFIELD_MEMBER(31, 31, reliable_write, raw);
  };

  /**
   * SD Specification Part 1 (Physical Layer Simplified Specification).
   * Table 4-31: Argument of ACMD6.
   */
  struct Arg_acmd6_sd_set_bus_width : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(0, 1, bus_width, raw);
    enum Bus_width
    {
      Bus_width_1bit = 0,
      Bus_width_4bit = 2,
    };
  };

  /**
   * SD Specifications Part 1 (Physical Layer Simplified Specification).
   * Figure 4-3: Argument of ACMD41.
   * See also Reg_ocr.
   */
  struct Arg_acmd41_sd_send_op : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(15, 23, voltrange_sd, raw);
    CXX_BITFIELD_MEMBER(15, 15, mv2700_2800, raw);
    CXX_BITFIELD_MEMBER(16, 16, mv2800_2900, raw);
    CXX_BITFIELD_MEMBER(17, 17, mv2900_3000, raw);
    CXX_BITFIELD_MEMBER(18, 18, mv3000_3100, raw);
    CXX_BITFIELD_MEMBER(19, 19, mv3100_3200, raw);
    CXX_BITFIELD_MEMBER(20, 20, mv3200_3300, raw);
    CXX_BITFIELD_MEMBER(21, 21, mv3300_3400, raw);
    CXX_BITFIELD_MEMBER(22, 22, mv3400_3500, raw);
    CXX_BITFIELD_MEMBER(23, 23, mv3500_3600, raw);
    CXX_BITFIELD_MEMBER(24, 24, s18r, raw);
    CXX_BITFIELD_MEMBER(28, 28, xpc, raw);         ///< true: 0.54W, else 0.36W
    CXX_BITFIELD_MEMBER(30, 30, hcs, raw);         ///< SD: Host capacity supp
    CXX_BITFIELD_MEMBER(31, 31, not_busy, raw);    ///< 0: Card power up busy
  };

  /**
   * SD Specifications Part E1
   * Figure 5-1: IO_RW_DIRECT command
   */
  struct Arg_cmd52_io_rw_direct : public Arg
  {
    using Arg::Arg;
    CXX_BITFIELD_MEMBER(0, 7, write_data, raw);
    CXX_BITFIELD_MEMBER(9, 25, address, raw);
    CXX_BITFIELD_MEMBER(27, 27, read_after_write, raw);
    CXX_BITFIELD_MEMBER(28, 30, function, raw);
    CXX_BITFIELD_MEMBER(31, 31, write, raw);
  };
};

} // namespace Emmc
