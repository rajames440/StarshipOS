/*
 * Copyright (C) 2020, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Driver base functionality.
 *
 * Most important the access to the MMIO registers and simple functions for
 * simple command execution without interrupts (required during setup).
 */

#pragma once

#include <functional>

#include <l4/drivers/hw_mmio_register_block>
#include <l4/re/mmio_space>

#include "cmd.h"
#include "mmc.h"
#include "mmio.h"
#include "util.h"

namespace Emmc {

typedef std::function<void(bool)> Receive_irq;

struct Drv_base
{
  void stats_wait_start()
  { _time_sleep -= Util::read_tsc(); }

  void stats_wait_done()
  { _time_sleep += Util::read_tsc(); }

  void delay(unsigned ms);

  // Statistics
  l4_uint64_t _time_busy = 0;
  l4_uint64_t _time_sleep = 0;
};

template <class Hw_drv>
struct Drv : public Drv_base
{
  using Hw_regs = L4drivers::Register_block<32>;

  explicit Drv(L4::Cap<L4Re::Dataspace> iocap,
               L4::Cap<L4Re::Mmio_space> mmio_space,
               l4_uint64_t mmio_base, l4_uint64_t mmio_size,
               Receive_irq receive_irq)
  : _regs(mmio_space.is_valid()
            ? Hw_regs(new Hw::Mmio_space_register_block<32>(
                            mmio_space, mmio_base, mmio_size))
            : Hw_regs(new Hw::Mmio_map_register_block<32>(
                            iocap, mmio_base, mmio_size))),
    _receive_irq(receive_irq)
  {}

  /** Return descriptor of currently active command. */
  Cmd *cmd_current() { return _cmd_queue.working(); }

  /** Create a new descriptor out of the descriptor list. */
  Cmd *cmd_create() { return _cmd_queue.create(); }

  /**
   * Submit a command to the controller and return immediately.
   *
   * \param cmd  Command context.
   */
  void cmd_submit_on_avail(Cmd *cmd)
  {
    Hw_drv &drv = static_cast<Hw_drv &>(*this);

    drv.cmd_wait_available(cmd, false);
    drv.cmd_submit(cmd);
  }

  /**
   * Submit a command to the controller and wait until the command completed
   * (either successfully or with error).
   *
   * \param cmd  Command context.
   */
  void cmd_exec(Cmd *cmd)
  {
    cmd_submit_on_avail(cmd);

    Hw_drv &drv = static_cast<Hw_drv &>(*this);
    drv.cmd_wait_cmd_finished(cmd, false);
    if (cmd->error())
      return;

    drv.cmd_wait_data_finished(cmd);
    if (cmd->error())
      return;

    if (cmd->cmd & Mmc::Rsp_present)
      drv.cmd_fetch_response(cmd);
  }

  bool cmd_queue_kick()
  {
    Cmd *cmd = _cmd_queue.working();
    if (cmd && cmd->status == Cmd::Ready_for_submit)
      {
        cmd_submit_on_avail(cmd);
        return true;
      }
    return false;
  }

  /** Return true if a bounce buffer was provided for this driver instance. */
  bool provided_bounce_buffer() const
  { return _bb_size != 0; }

  /** Return true if this memory region is accessible by the DMA engine. */
  bool dma_accessible(l4_uint64_t dma_addr, l4_size_t size)
  { return dma_addr <= _dma_limit && dma_addr + size - 1 <= _dma_limit; }

  /**
   * Perform the sdio reset, if necessary. The default is to not do anything.
   */
  void sdio_reset(Cmd*)
  { }

  Hw_regs     _regs;                    ///< Controller MMIO registers.
  Receive_irq _receive_irq;             ///< IRQ receive function.
  Cmd_queue   _cmd_queue;               ///< Command queue.

  L4Re::Dma_space::Dma_addr _bb_phys;   ///< Bounce buffer: DMA address.
  l4_addr_t _bb_virt = 0;               ///< Bounce buffer: virtual address.
  l4_size_t _bb_size = 0;               ///< Bounce buffer: size.
  L4Re::Dma_space::Dma_addr _dma_limit = ~0ULL;
                                        /// Largest device DMA-accessible address.
};

}; // namespace Emmc
