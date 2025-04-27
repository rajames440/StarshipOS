/*
 * Copyright (C) 2020, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file backend for SDHI used by RCar3.
 */

#include "cmd.h"
#include "cpg.h"
#include "debug.h"
#include "drv_sdhi.h"
#include "factory.h"
#include "mmc.h"
#include "util.h"

#include <l4/sys/kdebug.h>

namespace Emmc {

Sdhi::Sdhi(int nr,
           L4::Cap<L4Re::Dataspace> iocap,
           L4::Cap<L4Re::Mmio_space> mmio_space,
           l4_uint64_t mmio_base, l4_uint64_t mmio_size,
           L4Re::Util::Shared_cap<L4Re::Dma_space> const &,
           l4_uint32_t, Receive_irq receive_irq)
: Drv(iocap, mmio_space, mmio_base, mmio_size, receive_irq),
  warn(Dbg::Warn, "sdhi", nr),
  info(Dbg::Info, "sdhi", nr),
  trace(Dbg::Trace, "sdhi", nr)
{
  trace.printf("Assuming SDHI eMMC controller (VERSION=%08x), registers at %08llx.\n",
               Reg_version(_regs).raw, mmio_base);
}

void
Sdhi::init()
{
  // Disable DMA interrupts.
  Reg_sd_clk_ctrl sd_clk_ctrl;
  sd_clk_ctrl.set_divisor(4);
  sd_clk_ctrl.write(_regs);

  // Reset SD interface unit.
  Reg_soft_rst soft_rst;
  soft_rst.sdrst() = 0;
  soft_rst.write(_regs);
  soft_rst.sdrst() = 1;
  soft_rst.write(_regs);

  // Mask all SDIO interrupts.
  Reg_sdio_info1_mask sdio_info1_mask;
  sdio_info1_mask.iomsk() = 1;
  sdio_info1_mask.mexpub52() = 1;
  sdio_info1_mask.mexwt() = 1;
  sdio_info1_mask.write(_regs);

  // Enable SDIO interrupts.
  Reg_sdio_mode sdio_mode;
  sdio_mode.iomod() = 1;
  sdio_mode.write(_regs);

  // Reset channel 0 and 1.
  Reg_dm_cm_rst dm_cm_rst(_regs);
  dm_cm_rst.dtranrst() = 0;
  dm_cm_rst.write(_regs);
  dm_cm_rst.dtranrst() = 3;
  dm_cm_rst.write(_regs);

  // Reset SDIF mode (no HS400).
  Reg_sdif_mode().write(_regs);

  // Enable SD clock output.
  sd_clk_ctrl.sclken() = 1;
  sd_clk_ctrl.write(_regs);

  Reg_sd_info_mask sd_info_mask;
  sd_info_mask.disable_ints();
  sd_info_mask.write(_regs);

  Reg_dm_cm_info1_mask dm_cm_info1_mask;
  dm_cm_info1_mask.disable_ints();
  dm_cm_info1_mask.write(_regs);

  Reg_dm_cm_info2_mask dm_cm_info2_mask;
  dm_cm_info2_mask.disable_ints();
  dm_cm_info2_mask.write(_regs);

  Reg_host_mode host_mode(_regs);
  host_mode.set_bus_width(Reg_host_mode::Width_64bit);
  host_mode.write(_regs);
}

void
Sdhi::enable_dma(bool enable)
{
  if (!enable)
    Reg_dm_cm_info1().write(_regs);
  Reg_cc_ext_mode cc_ext_mode;
  cc_ext_mode.dmasdrw() = enable;
  cc_ext_mode.write(_regs);
}

/**
 * Disable all interrupt sources.
 */
void
Sdhi::mask_interrupts()
{
  Reg_sd_info_mask sd_info_mask;
  sd_info_mask.disable_ints();
  sd_info_mask.write(_regs);
}

Cmd *
Sdhi::handle_irq()
{
  Cmd *cmd = _cmd_queue.working();
  if (cmd)
    {
      Reg_sd_info sd_info(_regs);
      printf("handle_irq: info = %08x\n", sd_info.raw);

      if (cmd->status == Cmd::Progress_cmd)
        handle_irq_cmd(cmd, sd_info);

      if (cmd->status == Cmd::Progress_data)
        handle_irq_data(cmd, sd_info);

      if (sd_info.read(_regs))
        trace.printf("after handle_irq: info = \033[31m%08x\033[m\n",
                     sd_info.raw);

      if (cmd->status == Cmd::Success)
        cmd_fetch_response(cmd);
    }
  // else polling

  // for driver "bottom-half handling"
  return cmd;
}

void
Sdhi::handle_irq_cmd(Cmd *cmd, Reg_sd_info sd_info)
{
  (void)cmd;
  printf("handle_irq_cmd %08x\n", sd_info.raw);
  Reg_sd_info sd_info_ack(~0U);
  if (sd_info.info0() || sd_info.err6())
    {
      sd_info_ack.info0() = 0;
      sd_info_ack.err6() = 0;
      sd_info_ack.write(_regs);
      if (sd_info.err6())
        cmd->status = Cmd::Cmd_timeout;
      else if (sd_info.error())
        cmd->status = Cmd::Cmd_error;
      else if (cmd->flags.has_data())
        {
          cmd->status = Cmd::Progress_data;
          Reg_dm_cm_dtran_ctrl dtran_ctrl(_regs);
          dtran_ctrl.dm_start() = 1;
          dtran_ctrl.write(_regs);
        }
      else
        cmd->status = Cmd::Success;
    }
  printf("Status = %s\n", cmd->str_error());
}

void
Sdhi::handle_irq_data(Cmd *cmd, Reg_sd_info sd_info)
{
  (void)cmd;
  printf("handle_irq_data %08x\n", sd_info.raw);
  Reg_sd_info sd_info_ack(~0U);
  if (sd_info.bre() || sd_info.bwe())
    {
      sd_info_ack.bre() = 0;
      sd_info_ack.bwe() = 0;
      sd_info_ack.write(_regs);
    }

  bool done = false;
  if (cmd->cmd & Mmc::Dir_read)
    {
      // read
      done = true;
    }
  else
    {
      // write
      Reg_sd_info sd_info;
      if (sd_info.sclkdiven())
        done = true;
      if (!sd_info.cbsy())
        done = true;
    }

  if (done)
    {
      Reg_sd_info_mask sd_info_mask(_regs);
      sd_info_mask.imask2() = 1;
      sd_info_mask.write(_regs);

      Reg_dm_cm_info1().write(_regs);

      Reg_cc_ext_mode cc_ext_mode;
      cc_ext_mode.dmasdrw() = 0;
      cc_ext_mode.write(_regs);
      cmd->status = Cmd::Success;
    }

  if (sd_info.err1() || sd_info.err2() || sd_info.err5())
    cmd->status = Cmd::Data_error;
}

/**
 * Send an MMC command to the controller.
 */
void
Sdhi::cmd_submit(Cmd *cmd)
{
  if (cmd->status != Cmd::Ready_for_submit)
    L4Re::throw_error(-L4_EINVAL, "Invalid command submit status");

  Reg_sd_cmd sd_cmd;
  sd_cmd.cf() = cmd->cmd_idx();
  switch (cmd->cmd & Mmc::Rsp_mask)
    {
    case Mmc::Rsp_none: sd_cmd.mode() = Reg_sd_cmd::Resp_none; break;
    case Mmc::Resp_r1:  sd_cmd.mode() = Reg_sd_cmd::Resp_r1;   break;
    case Mmc::Resp_r1b: sd_cmd.mode() = Reg_sd_cmd::Resp_r1b;  break;
    case Mmc::Resp_r2:  sd_cmd.mode() = Reg_sd_cmd::Resp_r2;   break;
    case Mmc::Resp_r3:  sd_cmd.mode() = Reg_sd_cmd::Resp_r3;   break;
    default:
      L4Re::throw_error(-L4_EINVAL, "Unexpected response type");
      break;
    }

  if (cmd->flags.has_data())
    {
      Reg_sd_size sd_size;
      sd_size.len() = cmd->blocksize;
      if (sd_size.len() != cmd->blocksize)
        L4Re::throw_error(-L4_EINVAL, "Size of data blocks to transfer");
      sd_size.write(_regs);
      sd_cmd.md3() = 1;
      Reg_sd_stop sd_stop;
      if (cmd->blockcnt > 1)
        {
          sd_cmd.md5() = 1;
          // XXX disable auto CMD12
          sd_stop.sec() = 1;
        }
      sd_stop.write(_regs);
      Reg_sd_seccnt(cmd->blockcnt).write(_regs);
      sd_cmd.md4() = !!(cmd->cmd & Mmc::Dir_read);

      Reg_dm_cm_dtran_mode dtran_mode;
      dtran_mode.bus_width() = Reg_dm_cm_dtran_mode::Bus_64bits;
      dtran_mode.addr_mode() = Reg_dm_cm_dtran_mode::Incr_addr;
      dtran_mode.ch_num() = (cmd->cmd & Mmc::Dir_read)
                            ? Reg_dm_cm_dtran_mode::Ch_1_read
                            : Reg_dm_cm_dtran_mode::Ch_0_write;

      Reg_dm_cm_info1().write(_regs);

      Reg_cc_ext_mode cc_ext_mode;
      cc_ext_mode.dmasdrw() = 1;
      cc_ext_mode.write(_regs);

      dtran_mode.write(_regs);

      if (cmd->blocks)
        Reg_dm_dtran_addr(cmd->blocks->dma_addr).write(_regs);
      else
        Reg_dm_dtran_addr(cmd->data_phys).write(_regs);

      Reg_dm_cm_info1_mask dm_cm_info1_mask(_regs);
      Reg_dm_cm_info2_mask dm_cm_info2_mask(_regs);
      if (cmd->cmd & Mmc::Dir_read)
        {
          dm_cm_info1_mask.dtranend12_mask() = 0;
          dm_cm_info1_mask.dtranend11_mask() = 0;
          dm_cm_info2_mask.dtranerr1_mask() = 0;
        }
      else
        {
          dm_cm_info1_mask.dtranend0_mask() = 0;
          dm_cm_info2_mask.dtranerr0_mask() = 0;
        }
      dm_cm_info1_mask.write(_regs);
      dm_cm_info2_mask.write(_regs);
    }

  Reg_sd_info_mask sd_info_mask;
  sd_info_mask.enable_ints();
  sd_info_mask.write(_regs);

  Reg_sd_arg(cmd->arg).write(_regs);
  sd_cmd.write(_regs);

  if (cmd->cmd == Mmc::Cmd6_switch)
    trace.printf("Send \033[33mCMD%d / %d (arg=%08x) -- %s\033[m\n",
                 cmd->cmd_idx(), (cmd->arg >> 16) & 0xff, cmd->arg,
                 cmd->cmd_to_str().c_str());
  else
    trace.printf("Send \033[32mCMD%d (arg=%08x) -- %s\033[m\n",
                 cmd->cmd_idx(), cmd->arg, cmd->cmd_to_str().c_str());

  cmd->status = Cmd::Progress_cmd;

  if (cmd->cmd_idx() == 8 && cmd->flags.has_data())
    dump();
}

void
Sdhi::cmd_wait_available(Cmd const *, bool)
{
  // Reg_sd_info.cbsy()?
}

/**
 * Wait for completion of command send phase.
 */
void
Sdhi::cmd_wait_cmd_finished(Cmd *cmd, bool verbose)
{
  l4_uint64_t time = Util::read_tsc();
  while (cmd->status == Cmd::Progress_cmd)
    {
      _receive_irq(false);
      handle_irq_cmd(cmd, Reg_sd_info(_regs));
    }
  time = Util::read_tsc() - time;
  _time_sleep += time;
  l4_uint64_t us = Util::tsc_to_us(time);
  if ((verbose && us >= 1000) || cmd->error())
    {
      char const *s = cmd->error()
                        ? cmd->flags.expected_error()
                          ? " (failed, expected)"
                          : " \033[31m(failed)\033[m"
                        : "";
      info.printf("CMD%d took \033[1m%lluus%s.\033[m\n", cmd->cmd_idx(), us, s);
    }
}

/**
 * Wait for command completion.
 */
void
Sdhi::cmd_wait_data_finished(Cmd *cmd)
{
  l4_uint64_t time = Util::read_tsc();
  while (cmd->status == Cmd::Progress_data)
    {
      _receive_irq(true);
      handle_irq_data(cmd, Reg_sd_info(_regs));
    }
  time = Util::read_tsc() - time;
  _time_sleep += time;
  l4_uint64_t us = Util::tsc_to_us(time);
  if (us >= 1000)
    warn.printf("CMD%d data took \033[1m%lluus.\033[m\n", cmd->cmd_idx(), us);
}

/**
 * Fetch response after a command was successfully executed.
 */
void
Sdhi::cmd_fetch_response(Cmd *cmd)
{
  if (cmd->cmd & Mmc::Rsp_136_bits)
    {
      Reg_sd_rsp10 rsp10(_regs);
      Reg_sd_rsp32 rsp32(_regs);
      Reg_sd_rsp54 rsp54(_regs);
      Reg_sd_rsp76 rsp76(_regs);
      cmd->resp[0] = (rsp76.raw << 8) | (rsp54.raw >> 24);
      cmd->resp[1] = (rsp54.raw << 8) | (rsp32.raw >> 24);
      cmd->resp[2] = (rsp32.raw << 8) | (rsp10.raw >> 24);
      cmd->resp[3] = (rsp10.raw << 8);
    }
  else
    {
      cmd->resp[0] = Reg_sd_rsp10(_regs).raw;
      cmd->flags.has_r1_response() = 1;
      Mmc::Device_status s = cmd->mmc_status();
      if (s.current_state() != s.Transfer)
        trace.printf("\033[35mCommand response R1 (%s)\033[m\n", s.str());
    }
}

void
Sdhi::set_clock_and_timing(l4_uint32_t freq, Mmc::Timing timing, bool strobe)
{
  (void)timing;
  (void)strobe;

  clock_disable();
  if (!freq)
    {
      info.printf("\033[33mClock disabled.\033[m\n");
      return;
    }

  set_clock(freq);
  clock_enable();
}

void
Sdhi::set_bus_width(Mmc::Bus_width bus_width)
{
  Reg_sd_option op(_regs);
  op.set_bus_width(bus_width);
  op.write(_regs);

  info.printf("\033[33mSet bus width to %s.\033[m\n", op.str_bus_width());
}

void
Sdhi::clock_disable()
{
  Reg_sd_clk_ctrl ctrl(_regs);
  ctrl.sclken() = 0;
  ctrl.write(_regs);
}

void
Sdhi::clock_enable()
{
  Reg_sd_clk_ctrl ctrl(_regs);
  ctrl.sclken() = 1;
  ctrl.write(_regs);
}

void
Sdhi::set_clock(l4_uint32_t freq)
{
  l4_uint32_t clk_div = 0x80;

  if (freq < 200000000)
    {
      for (l4_uint32_t real_clock = 200000000 / 512;
           freq >= (real_clock << 1);
           clk_div >>= 1, real_clock <<= 1)
        ;
    }
  else
    clk_div = 0xff;

  Reg_sd_clk_ctrl ctrl(_regs);
  ctrl.div() = clk_div;
  ctrl.write(_regs);

  info.printf("\033[33mSet clock to %s (host=%s, divisor=%d).\033[m\n",
              Util::readable_freq(freq).c_str(),
              Util::readable_freq(200000000).c_str(),
              Reg_sd_clk_ctrl(_regs).divisor());
}

void
Sdhi::dump() const
{
  warn.printf("Registers:\n");
  for (unsigned i = 0; i < 0xF0; i += 8)
    warn.printf("  %04x: %08x\n", i, l4_uint32_t{_regs[i]});
  warn.printf("  %04x: %08x\n", 0x360, l4_uint32_t{_regs[0x360]});
  for (unsigned i = 0x380; i < 0x3a0; i += 8)
    warn.printf("  %04x: %08x\n", i, l4_uint32_t{_regs[i]});
  for (unsigned i = 0x800; i < 0x8e0; i += 8)
    warn.printf("  %04x: %08x\n", i, l4_uint32_t{_regs[i]});
}

} // namespace Emmc

namespace {

using namespace Emmc;

static Rcar3_cpg *cpg;

static void
init_cpg()
{
  if (!cpg)
    cpg = new Rcar3_cpg();
  cpg->enable_clock(3, 12);
  cpg->enable_register(Rcar3_cpg::Sd2ckcr, 0x201);
}

/**
 * SDHI found in RCar3.
 */
struct F_sdhi_rcar3 : Factory
{
  cxx::Ref_ptr<Device_factory::Device_type>
  create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
         L4::Cap<L4::Icu> icu, L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
         unsigned max_seg, Device_type_disable dt_disable)
  {
    L4::Cap<L4Re::Mmio_space> mmio_space;
    init_cpg();
    return cxx::make_ref_obj<Device<Sdhi>>(
             nr, mmio_addr, mmio_size, iocap, mmio_space, irq_num, irq_mode,
             icu, dma, registry, host_clock, max_seg, dt_disable);
  }
};

static F_sdhi_rcar3 f_sdhi_rcar3;
static Device_type_nopci t_sdhi_rcar3{"renesas,sdhi-r8a7795", &f_sdhi_rcar3};

/**
 * SDHI found in RCar3 connected to RCar3 emulator.
 */
struct F_sdhi_emu : Factory
{
  cxx::Ref_ptr<Device_factory::Device_type>
  create(unsigned nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
         L4::Cap<L4Re::Dataspace> iocap, int irq_num, L4_irq_mode irq_mode,
         L4::Cap<L4::Icu> icu, L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
         L4Re::Util::Object_registry *registry, l4_uint32_t host_clock,
         unsigned max_seg, Device_type_disable dt_disable) override
  {
    auto mmio_space = L4::cap_dynamic_cast<L4Re::Mmio_space>(iocap);
    init_cpg();
    return cxx::make_ref_obj<Device<Sdhi>>(
             nr, mmio_addr, mmio_size, iocap, mmio_space, irq_num, irq_mode,
             icu, dma, registry, host_clock, max_seg, dt_disable);
  }
};

static F_sdhi_rcar3 f_sdhi_emu;
static Device_type_nopci t_sdhi_emu{"renesas,sdhi-r8a7796", &f_sdhi_emu};

} // namespace
