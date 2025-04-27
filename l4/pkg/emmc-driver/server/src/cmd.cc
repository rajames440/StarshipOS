/*
 * Copyright (C) 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "cmd.h"

namespace Emmc {

void Cmd::work_done()
{
  queue->cmd_work_done(this);
}

void Cmd::destruct()
{
  queue->cmd_destruct(this);

  // invalidate
  status = Error;
  cmd = ~0U;
  arg = 0;
  flags.reset();
  // invalidating this callback is actually important
  cb_io  = 0;
}

int Cmd::nr() const
{
  if (!queue)
    return -1;

  return this - queue->cmds();
}

char const *Cmd::str_error() const
{
  switch (status)
    {
    case Success:          return "No error";
    case Uninitialized:    return "Uninitialized";
    case Ready_for_submit: return "Ready for submit";
    case Error:            return "General error";
    case Progress_cmd:     return "Command phase";
    case Progress_data:    return "Data phase";
    case Data_partial:     return "Data partially transferred";
    case Tuning_progress:  return "Tuning in progress";
    case Cmd_timeout:      return "Command phase timeout";
    case Cmd_error:        return "Command phase error";
    case Data_error:       return "Data transfer error";
    case Tuning_failed:    return "Tuning failed";
    default:               return "Unknown error";
    }
}

std::string Cmd::str_status() const
{
  if (error())
    return str_error();

  if (flags.has_r1_response())
    {
      Mmc::Device_status s(resp[0]);
      if (s.switch_error())
        {
          char str[32];
          snprintf(str, sizeof(str), "SWITCH error (%08x)", s.raw);
          return str;
        }
    }
  return "success";
}

std::string Cmd::cmd_to_str() const
{
  switch (cmd_idx())
    {
    case 0:
      switch (arg)
        {
        case 0x00000000: return "GO_IDLE_STATE";
        case 0xf0f0f0f0: return "GO_PRE_IDLE_STATE";
        case 0xfffffffa: return "BOOT_INITIATION";
        default:         return "CMD0_unknown";
        }
    case  1: return "SEND_OP_COND";
    case  2: return "ALL_SEND_CID";
    case  3: return raw() == Mmc::Cmd3_set_relative_addr ? "SET_RELATIVE_ADDR"
                                                         : "SEND_RELATIVE_ADDR";
    case  4: return "SET_DSR";
    case  5: return raw() == Mmc::Cmd5_sleep_awake ? "SLEEP_AWAKE"
                                                   : "IO_SEND_OP_COND";
    case  6: return raw() == Mmc::Cmd6_switch_func ? "SWITCH_FUNC"
                                                   : "SWITCH";
    case  7: return "SELECT/DESELECT_CARD";
    case  8: return raw() == Mmc::Cmd8_send_ext_csd ? "SEND_EXT_CSD"
                                                    : "SEND_IF_COND";
    case  9: return "SEND_CSD";
    case 10: return "SEND_CID";
    case 11: return "CMD11_obsolete";
    case 12: return "STOP_TRANSMISSION";
    case 13: return "SEND_STATUS";
    case 14: return "BUSTEST_R";
    case 15: return "GO_INACTIVE_STATE";
    case 18: return "READ_MULTIPLE_BLOCK";
    case 19: return "SEND_TUNING_BLOCK"; // SD
    case 21: return "SEND_TUNING_BLOCK_HS200"; // eMMC
    case 23: return "SET_BLOCK_COUNT";
    case 24: return "WRITE_BLOCK";
    case 25: return "WRITE_MULTIPLE_BLOCK";
    case 26: return "PROGRAM_CID";
    case 27: return "PROGRAM_CSD";
    case 28: return "SET_WRITE_PROT";
    case 29: return "CLR_WRITE_PROT";
    case 30: return "SEND_WRITE_PROT";
    case 31: return "SEND_WRITE_PROT_TYPE";
    case 35: return "ERASE_GROUP_START";
    case 36: return "ERASE_GROUP_END";
    case 38: return "ERASE";
    case 39: return "FAST_IO";
    case 40: return "GO_IRQ_STATE";
    case 41: return flags.app_cmd() ? "SD_SEND_OP_COND"  // ACMD41, SD-only
                                    : "CMD_unknown";
    case 42: return "LOCK_UNLOCK";
    case 44: return "QUEUED_TASK_PARAMS";
    case 45: return "QUEUED_TASK_ADDRESS";
    case 46: return "EXECUTE_READ_TASK";
    case 47: return "EXECUTE_WRITE_TASK";
    case 48: return "CMDQ_TASK_MGMT";
    case 49: return "SET_TIME";
    case 51: return flags.app_cmd() ? "SEND_SCR"         // ACMD51, SD-only
                                    : "CMD_unknown";
    case 52: return "IO_RW_DIRECT";                      // SDIO
    case 53: return "PROTOCOL_RD";
    case 54: return "PROTOCOL_WR";
    case 55: return "APP_CMD";
    case 56: return "GEN_CMD";
    case 60: return "RW_MULTIPLE_REGISTER";
    case 61: return "RW_MULTIPLE_BLOCK";
    default: return "CMD_unknown";
    }
}
} // namespace Emmc
