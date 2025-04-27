/*
 * Copyright (C) 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * eMMC command handling.
 *
 * Prerequisite for command queuing.
 */

#pragma once

#include <string>

#include <l4/re/error_helper>
#include <l4/libblock-device/types.h>
#include <l4/libblock-device/errand.h>

#include "mmc.h"

namespace Emmc {

class Cmd_queue;

/**
 * eMMC command context.
 * Independent from the eMMC driver.
 */
class Cmd
{
public:
  using Callback_io = Block_device::Inout_callback;

  using Block = Block_device::Inout_block;

  enum Status : int
  {
    Success = 0,                ///< Command finished successfully.

    // no-error conditions
    Progress_cmd = 1,           ///< Executing command phase.
    Progress_data = 2,          ///< Executing data phase.
    Data_partial = 3,           ///< Data partially read, continue transfer.
    Tuning_progress = 4,        ///< Tuning in progress.

    // errors
    Uninitialized = -1,         ///< Command was just created.
    Ready_for_submit = -2,      ///< Asynchronous command not yet submitted.
    Error = -3,                 ///< General unspecified error.
    Cmd_timeout = -4,           ///< Timeout during command phase.
    Cmd_error = -5,             ///< Error during command phase.
    Data_error = -6,            ///< Error during data phase.
    Tuning_failed = -7,         ///< Tuning failed.
  };

  enum Flag_auto_cmd23
  {
    No_auto_cmd23 = 0,
    Do_auto_cmd23 = 1,
  };

  bool error() const
  { return status < 0; }

  void check_error(char const *err_str)
  {
    if (error())
      L4Re::throw_error(-L4_EIO, err_str);
  }

  bool progress() const
  { return status == Progress_cmd || status == Progress_data; }

  struct Flags
  {
  private:
    l4_uint32_t _raw = 0;

  public:
    /// Bounce buffer used for this request.
    CXX_BITFIELD_MEMBER(10, 10, read_from_bounce_buffer, _raw);
    /// The previous command was CMD55 (APP_CMD). Only for logging.
    CXX_BITFIELD_MEMBER(9, 9, app_cmd, _raw);
    /// Enable Auto CMD23 for this command during submission.
    CXX_BITFIELD_MEMBER(8, 8, auto_cmd23, _raw);
    /// Inout: 1 = read, 0 = write.
    CXX_BITFIELD_MEMBER(7, 7, inout_read, _raw);
    /// Inout: CMD12 required after end of this Inout command, either automatic
    /// explicit -- that's up to the driver.
    CXX_BITFIELD_MEMBER(6, 6, inout_cmd12, _raw);
    /// Inout: This is an inout command.
    CXX_BITFIELD_MEMBER(5, 5, inout, _raw);
    /// Error is not unlikely, decrease logging verbosity.
    CXX_BITFIELD_MEMBER(4, 4, expected_error, _raw);
    /// Response was fetched.
    CXX_BITFIELD_MEMBER(3, 3, has_r1_response, _raw);
    /// Execute CMD13 (STATUS) after SWITCH (CMD6).
    CXX_BITFIELD_MEMBER(2, 2, status_after_switch, _raw);
    /// Command transfers data (CMD8, CMD17, CMD18, CMD24, CMD25).
    CXX_BITFIELD_MEMBER(1, 1, has_data, _raw);
    /// Command was created with create().
    CXX_BITFIELD_MEMBER(0, 0, enqueued, _raw);

    /** Reset all flags except 'enqueued'. */
    void reset()
    { _raw &= 1U; }
  };

  explicit Cmd() {}

  /** Number within queue (only for logging / debugging). */
  int nr() const;

  /** Command without argument. */
  void init(l4_uint32_t cmd_val)
  {
    status = Ready_for_submit;
    cmd = cmd_val;
    arg = 0;
    flags.reset();
  }

  /** Mark a command as ACMD (previous command was CMD55). */
  void mark_app_cmd()
  { flags.app_cmd() = true; }

  /** Command with argument. */
  void init_arg(l4_uint32_t cmd_val, l4_uint32_t arg_val)
  {
    cmd = cmd_val;
    arg = arg_val;
    flags.reset();
    status = Ready_for_submit;
  }

  /** Command for single data transfer (CMD8). */
  void init_data(l4_uint32_t cmd_val, l4_uint32_t arg_val,
                 l4_uint32_t blocksize_val, l4_uint64_t data_phys_val,
                 l4_addr_t data_virt_val)
  {
    cmd = cmd_val;
    arg = arg_val;
    flags.reset();
    flags.has_data() = 1;
    blockcnt = 1;
    blocksize = blocksize_val;
    if (data_phys & 0xffffffff00000000ULL)
      L4Re::throw_error(-L4_ENOMEM, "Physical address beyond 4G");
    data_phys = data_phys_val & 0xffffffff;
    data_virt = data_virt_val;
    blocks = nullptr;
    status = Ready_for_submit;
  }

  /** Command for handling multiple MMC commands for inout(). */
  void init_inout(l4_uint64_t sector_val, Block const *blocks_val,
                  Callback_io cb_io_val, bool inout_read)
  {
    cmd = 0;
    flags.reset();
    flags.inout() = 1;
    flags.inout_read() = inout_read;
    sector = sector_val;
    sectors = 0;
    blocks = blocks_val;
    cb_io = cb_io_val;
  }

  /** Inout command without data (CMD23). */
  void reinit_inout_nodata(l4_uint32_t cmd_val, l4_uint32_t arg_val)
  {
    cmd = cmd_val;
    arg = arg_val;
    flags.has_data() = 0;
    status = Ready_for_submit;
  }

  /** Inout command with data (CMD17/CMD18/CMD24/CMD25. */
  void reinit_inout_data(l4_uint32_t cmd_val, l4_uint32_t arg_val,
                         l4_uint32_t blockcnt_val, l4_uint32_t blocksize_val,
                         Flag_auto_cmd23 auto_cmd23)
  {
    cmd = cmd_val;
    arg = arg_val;
    blockcnt = blockcnt_val;
    blocksize = blocksize_val;
    flags.has_data() = 1;
    flags.auto_cmd23() = auto_cmd23;
    status = Ready_for_submit;
  }

  l4_uint32_t cmd_idx() const
  { return cmd & Mmc::Idx_mask; }

  l4_uint32_t cmd_type() const
  { return cmd & Mmc::Type_mask; }

  l4_uint32_t raw() const
  { return cmd; }

  /** Show current command as readable string. */
  std::string cmd_to_str() const;

  /** Show current status as readable string. */
  char const *str_error() const;

  /** Show MMC status as readable string. */
  std::string str_status() const;

  /** Return the MMC device status for a command with an MMC R1 response. */
  Mmc::Device_status mmc_status() const
  {
    if (!flags.has_r1_response())
      L4Re::throw_error(-L4_EINVAL, "Status without response");
    return Mmc::Device_status(resp[0]);
  }

  /**
   * Return true if there was a switch error (corresponding bit in MMC
   * device status set).
   */
  bool switch_error() const
  {
    return (status == Cmd::Success
            && flags.has_r1_response()
            && Mmc::Device_status(resp[0]).switch_error());
  }

  /** Command no longer active so no related interrupts any longer. */
  void work_done();

  /** Free this command for further usage. */
  void destruct();

  /*
   * Certain values (sector number, sector count) are 32-bit values because
   * the eMMC protocol uses 32-bit values there as well. The physical address
   * is currently 32-bit because we are using the SDMA engine.
   */
  Status       status;          ///< Command status.
  Status       status_cmd12;    ///< Status of command prior to CMD12.
  l4_uint32_t  cmd;             ///< MMC command value (see class Mmc).
  l4_uint32_t  arg;             ///< MMC command argument.
  Flags        flags;           ///< See Flags.
  l4_uint32_t  blockcnt;        ///< Number of blocks if flags.has_data = true.
  l4_uint32_t  blocksize;       ///< Block size if flags.has_data = true.
  l4_uint32_t  data_phys;       ///< Physical address if flags.has_data = true.
  l4_addr_t    data_virt;       ///< Only for certain MMIO requests.
  l4_uint32_t  resp[4];         ///< 16 bytes = 128 bits.

  // inout()
  l4_uint32_t  sector;          ///< Current sector on medium.
  l4_uint32_t  sectors;         ///< Overall number of transferred sectors.
  Block        const *blocks;   ///< See inout().

  // internal
  Cmd_queue    *queue = nullptr;

  Callback_io  cb_io = 0;       ///< Inout callback (inout()).
};

/**
 * Simple queue of \c Cmd objects allowing queue commands on the host side.
 */
class Cmd_queue
{
public:
  enum { Entries = 32 };

  explicit Cmd_queue()
  {
    for (unsigned i = 0; i < Entries; ++i)
      _cmds[i].queue = this;
  }

  bool is_full() const
  { return wrap_around(_create + 1) == _destruct; }

  Cmd *create()
  {
    if (is_full())
      return nullptr;
    Cmd *cmd = &_cmds[_create];
    _create = wrap_around(_create + 1);
    if (cmd->flags.enqueued())
      L4Re::throw_error(-L4_EINVAL, "Command queue entry not destroyed");
    cmd->flags.enqueued() = 1;
    cmd->status = Cmd::Uninitialized;
    return cmd;
  }

  Cmd *working()
  { return _working == _create ? nullptr : &_cmds[_working]; }

  unsigned num_work() const
  {
    unsigned cnt = 0;
    for (unsigned w = _working; w != _create; w = wrap_around(w + 1), ++cnt)
      ;
    return cnt;
  }

  void cmd_work_done(Cmd *cmd)
  {
    if (working() != cmd)
      L4Re::throw_error(-L4_EINVAL, "Queue disorder (working != cmd).");
    _working = wrap_around(_working + 1);
  }

  void cmd_destruct(Cmd *cmd)
  {
    if (!cmd->flags.enqueued())
      L4Re::throw_error(-L4_EINVAL, "Command queue entry was not created.");
    cmd->flags.enqueued() = 0;
    if (_destruct == _working)
      L4Re::throw_error(-L4_EINVAL, "Queue disorder (destruct == working).");
    _destruct = wrap_around(_destruct + 1);
  }

  Cmd const *cmds() const
  { return _cmds; }

private:
  unsigned wrap_around(unsigned i) const
  { return i % Entries; }

  Cmd _cmds[Entries];
  unsigned _create = 0;
  unsigned _working = 0;
  unsigned _destruct = 0;
};

} // namespace Emmc
