/*
 * Copyright (C) 2020-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/re/mmio_space>
#include <l4/sys/kip.h>

#include "device.h"
#include "debug.h"
#include "inout_buffer.h"
#include "util.h"

namespace Emmc {

enum
{
  KHz = 1000,
  MHz = 1000000,
};

//Mmc::Reg_ecsd::Ec196_device_type Device<Driver>::_device_type_disable(0);

template <class Driver>
std::string
Device<Driver>::readable_product(std::string const s)
{
  std::string s1;
  size_t l = s.length();
  for (unsigned i = 0; i < l; ++i)
    if (Util::printable(s[i]) && (s[i] != ' ' || (i < l-1 && s[i + 1] != ' ')))
      s1 += s[i];
  return s1;
}

template <class Driver>
Device<Driver>::Device(int nr, l4_uint64_t mmio_addr, l4_uint64_t mmio_size,
                       L4::Cap<L4Re::Dataspace> iocap,
                       L4::Cap<L4Re::Mmio_space> mmio_space,
                       int irq_num, L4_irq_mode irq_mode, L4::Cap<L4::Icu> icu,
                       L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma,
                       L4Re::Util::Object_registry *registry,
                       l4_uint32_t host_clock, int max_seg,
                       Device_type_disable dt_disable)
: _drv(nr, iocap, mmio_space, mmio_addr, mmio_size, dma, host_clock,
       [this](bool is_data) { receive_irq(is_data); }),
  _irq_num(irq_num),
  _irq_mode(irq_mode),
  _icu(icu),
  _dma(dma),
  _max_seg(max_seg),
  _registry(registry),
  _io_buf("iobuf", 512, _dma,
          L4Re::Dma_space::Direction::From_device,
          L4Re::Rm::F::Cache_uncached),
  _ecsd(*_io_buf.get<Mmc::Reg_ecsd const >()),
  warn(Dbg::Warn, "device", nr),
  info(Dbg::Info, "device", nr),
  trace(Dbg::Trace, "device", nr),
  trace2(Dbg::Trace2, "device", nr),
  _device_type_disable(dt_disable)
{
  _drv.mask_interrupts();

  if (!_drv.dma_accessible(_io_buf.pget(), _io_buf.size()))
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "IO buffer at %08llx-%08llx not accessible by DMA",
                          _io_buf.pget(), _io_buf.pget() + _io_buf.size());

  L4Re::chkcap(_irq = L4Re::Util::cap_alloc.alloc<L4::Irq>(),
               "Allocate IRQ capability slot.");
  L4Re::chksys(L4Re::Env::env()->factory()->create(_irq),
               "Create IRQ capability at factory.");

  L4Re::chksys(_icu->set_mode(_irq_num, _irq_mode), "Set IRQ mode.");

  int ret = L4Re::chksys(l4_error(_icu->bind(_irq_num, _irq)),
                         "Bind interrupt to ICU.");
  _irq_unmask_at_icu = ret == 1;

  if (trace.is_active())
    {
      auto cb = std::bind(&Device<Driver>::show_statistics, this);
      Block_device::Errand::schedule(cb, Stats_delay_us);
    }

  if (_irq_unmask_at_icu)
    _icu->unmask(_irq_num);
  else
    _irq->unmask();

  claim_bounce_buffer("bbds");

  info.printf("\033[33mMax request size %s%s.\033[m\n",
              Util::readable_size(max_size()).c_str(),
              max_size() < Max_size ? " (limited by bounce buffer / max_seg)" : "");
}

template <class Driver>
void
Device<Driver>::claim_bounce_buffer(char const *cap_name)
{
  auto *env = L4Re::Env::env();
  auto cap = env->get_cap<L4Re::Dataspace>(cap_name);
  if (!cap.is_valid())
    return;

  if (!_drv.bounce_buffer_if_required())
    {
      warn.printf("\033[31;1mBounce buffer provided but not used by driver.\033[m\n");
      return;
    }

  l4_size_t size = cap->size();
  if (size < (64 << 10))
    L4Re::throw_error(-L4_EINVAL, "Bounce buffer smaller than 64K");

  L4Re::Dma_space::Dma_addr phys;
  L4Re::chksys(_dma->map(L4::Ipc::make_cap_rw(cap), 0, &size,
                         L4Re::Dma_space::Attributes::None,
                         L4Re::Dma_space::Direction::Bidirectional,
                         &phys),
               "Resolve physical address of bounce buffer");

  if (size != cap->size())
    L4Re::throw_error(-L4_EINVAL, "Bounce buffer contiguous");

  auto rm = env->rm();
  L4Re::chksys(rm->attach(&_bb_region, size,
                          L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW
                          | L4Re::Rm::F::Cache_normal,
                          L4::Ipc::make_cap_rw(cap), 0, L4_PAGESHIFT),
               "Attach bounce buffer");

  // We should have at least one page per segment
  if (size / _max_seg < L4_PAGESIZE)
    L4Re::throw_error(-L4_EINVAL, "Bounce buffer is too small for max seg count");

  _drv._bb_size = size;
  _drv._bb_phys = phys;
  _drv._bb_virt = (l4_addr_t)_bb_region.get();

  if (!_drv.dma_accessible(phys, size))
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "Bounce buffer at %08llx-%08llx not accessible by DMA",
                          phys, phys + size);

  warn.printf("\033[31;1mUsing bounce buffer of %s @ %08llx if required.\033[m\n",
              Util::readable_size(size).c_str(), phys);
}

template <class Driver>
void
Device<Driver>::show_statistics()
{
  l4_cpu_time_t time = l4_kip_clock(l4re_kip());
  if (_stat_ints)
    info.printf("%llu ints/s\n", _stat_ints * 1000000 / (time - _stat_time));
  _stat_time = time;
  _stat_ints = 0;
  auto cb = std::bind(&Device<Driver>::show_statistics, this);
  Block_device::Errand::schedule(cb, Stats_delay_us);
}

template <class Driver>
void
Device<Driver>::reset()
{
  warn.printf("\033[31;mreset\033\n\n");
}

template <class Driver>
int
Device<Driver>::dma_map_all(Block_device::Mem_region *region, l4_addr_t offset,
                            l4_size_t num_sectors, L4Re::Dma_space::Direction,
                            L4Re::Dma_space::Dma_addr *phys)
{
  if (!region->dma_info)
    {
      l4_size_t ds_size = region->ds()->size();
      L4Re::Dma_space::Dma_addr addr;
      auto ret = _dma->map(L4::Ipc::make_cap_rw(region->ds()), 0,
                           &ds_size, L4Re::Dma_space::Attributes::None,
                           L4Re::Dma_space::Direction::Bidirectional, &addr);
      if (ret < 0 || ds_size < num_sectors * sector_size())
        {
          *phys = 0;
          warn.printf("Cannot resolve physical address (ret = %ld, %zu < %zu).\n",
                      ret, ds_size, num_sectors * sector_size());
          return -L4_ENOMEM;
        }

      auto device = cxx::Ref_ptr<Block_device::Device>(this);
      auto dma_info =
        cxx::make_unique<Emmc::Dma_info<Driver>>(addr, ds_size, device);
      region->dma_info =
        cxx::unique_ptr<Block_device::Dma_region_info>(dma_info.release());
    }

  auto *dma_info = static_cast<Dma_info<Driver> *>(region->dma_info.get());
  *phys = dma_info->addr + offset - region->ds_offset();
  return L4_EOK;
}

template <class Driver>
int
Device<Driver>::dma_map_single(Block_device::Mem_region *region, l4_addr_t offset,
                               l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
                               L4Re::Dma_space::Dma_addr *phys)
{
  if (Dma_map_workaround)
    {
      auto ds = region->ds();
      auto me_ds_offs = _ds_offs_map.find(ds.cap());
      if (me_ds_offs != _ds_offs_map.end())
        {
          auto me_addr = me_ds_offs->second.find(offset);
          if (me_addr != me_ds_offs->second.end())
            {
              if (num_sectors != me_addr->second.sectors)
                warn.printf("\033[37;41;1mMAP %08lx/%08lx size mismatch %08zx/%08zx -- ignoring!\n",
                            ds.cap(), offset, me_addr->second.sectors, num_sectors);
              ++(me_addr->second.refcnt);
              *phys = me_addr->second.phys;
              return L4_EOK;
            }
        }
    }

  l4_size_t ds_size = num_sectors * sector_size();

  auto ret = _dma->map(L4::Ipc::make_cap_rw(region->ds()), offset,
                       &ds_size, L4Re::Dma_space::Attributes::None,
                       dir, phys);
  if (ret < 0 || ds_size < num_sectors * sector_size())
    {
      *phys = 0;
      warn.printf("Cannot resolve physical address (ret = %ld, %zu < %zu).\n",
                  ret, ds_size, num_sectors * sector_size());
      return -L4_ENOMEM;
    }

  if (Dma_map_workaround)
    {
      auto ds = region->ds();
      Phys_entry p = { *phys, num_sectors, 1 };
      _ds_offs_map[ds.cap()].emplace(std::make_pair(offset, p));
      Ds_offs_entry d = { ds.cap(), offset };
      _phys_map.emplace(std::make_pair(*phys, d));
    }

  return L4_EOK;
}

template <class Driver>
int
Device<Driver>::dma_map(Block_device::Mem_region *region, l4_addr_t offset,
                        l4_size_t num_sectors, L4Re::Dma_space::Direction dir,
                        L4Re::Dma_space::Dma_addr *phys)
{
  if (_dma_map_all)
    return dma_map_all(region, offset, num_sectors, dir, phys);
  else
    return dma_map_single(region, offset, num_sectors, dir, phys);
}

template <class Driver>
void
Device<Driver>::dma_unmap_region(Dma_info<Driver> *dma_info)
{
  auto ret = _dma->unmap(dma_info->addr, dma_info->size,
                         L4Re::Dma_space::Attributes::None,
                         L4Re::Dma_space::Direction::Bidirectional);
  if (ret < 0)
    Dbg::info().printf("Failed to unmap (ret = %ld, addr = %llx, size = %zu)\n",
                       ret, dma_info->addr, dma_info->size);
}

template <class Driver>
int
Device<Driver>::dma_unmap_all(L4Re::Dma_space::Dma_addr, l4_size_t,
                              L4Re::Dma_space::Direction)
{
  return L4_EOK;
}

template <class Driver>
int
Device<Driver>::dma_unmap_single(L4Re::Dma_space::Dma_addr phys,
                                 l4_size_t num_sectors,
                                 L4Re::Dma_space::Direction dir)
{
  if (Dma_map_workaround)
    {
      auto me_phys = _phys_map.find(phys);
      if (me_phys == _phys_map.end())
        {
          warn.printf("\033[37;42;1mUNMAP %08llx not found in phys_map!\033[m\n", phys);
          return -L4_ENOENT;
        }
      auto me_ds_offs = _ds_offs_map.find(me_phys->second.ds);
      if (me_ds_offs == _ds_offs_map.end())
        {
          warn.printf("\033[37;42;1mUNMAP %08llx not found in ds_offs_map!\033[m\n", phys);
          return -L4_ENOENT;
        }
      auto me_addr = me_ds_offs->second.find(me_phys->second.offset);
      if (me_addr == me_ds_offs->second.end())
        {
          warn.printf("\033[37;42;1mUNMAP %08llx not found in offs_map!\033[m\n", phys);
          return -L4_ENOENT;
        }
      if (num_sectors != me_addr->second.sectors)
        warn.printf("\033[37;42;1mUNMAP %08llx size mismatch %08zx/%08zx -- ignoring\n",
                    phys, me_addr->second.sectors, num_sectors);
      if (me_addr->second.refcnt > 1)
        {
          --(me_addr->second.refcnt);
          return L4_EOK;
        }
      me_ds_offs->second.erase(me_addr);
      if (me_ds_offs->second.empty())
        _ds_offs_map.erase(me_ds_offs);

      _phys_map.erase(me_phys);
    }

  return _dma->unmap(phys, num_sectors * sector_size(),
                     L4Re::Dma_space::Attributes::None, dir);
}

template <class Driver>
int
Device<Driver>::dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t num_sectors,
                          L4Re::Dma_space::Direction dir)
{
  if (_dma_map_all)
    return dma_unmap_all(phys, num_sectors, dir);
  else
    return dma_unmap_single(phys, num_sectors, dir);
}


template <class Driver>
int
Device<Driver>::inout_data(l4_uint64_t sector,
                           Block_device::Inout_block const &blocks,
                           Block_device::Inout_callback const &cb,
                           L4Re::Dma_space::Direction dir)
{
  try
    {
      Cmd *cmd = _drv.cmd_create();
      if (!cmd)
        return -L4_EBUSY;
      cmd->cb_io = cb;

      bool inout_read = dir == L4Re::Dma_space::Direction::From_device;

      unsigned segments = 0;
      for (Block_device::Inout_block const *b = &blocks; b; b = b->next.get())
        {
          l4_size_t size = b->num_sectors * sector_size();
          if (size > max_size())
            {
              warn.printf("num_sectors=%u, sector_size=%zu, size=%zx, max_size=%zx\n",
                          b->num_sectors, sector_size(), size, max_size());
              L4Re::throw_error(-L4_EINVAL, "Segment size in inout_data()");
            }
          ++segments;
        }

      // enforced in Block_device::Virtio_client::build_inout_blocks()
      assert(segments <= max_segments());

      cmd->init_inout(sector, &blocks, cb, inout_read);

      if (_drv.dma_adma2())
        {
          /* For all blocks together, do a single CMD23 (set_block_count_adma2())
           * followed by a single CMD18/CMD25 (handle_irq_inout_adma2()). */
          set_block_count_adma2(cmd);
        }
      else
        {
          /* For every block do CMD23 followed by CMD18/CMD25. */
          transfer_block_sdma(cmd);
        }

      cmd_queue_kick();
    }
  catch (L4::Runtime_error const &e)
    {
      warn.printf("inout_data fails: %s: %s.\n", e.str(), e.extra_str());
      // -L4_EBUSY is only appropriate in certain cases (for example, there is
      // currently no free command slot), therefore rather enforce an IO error.
      return -L4_EINVAL;
    }

  return L4_EOK;
}

// Execute the SWITCH command to flush the device cache synchronously.
template <class Driver>
int
Device<Driver>::flush(Block_device::Inout_callback const &cb)
{
  info.printf("\033[32mflush\033[m\n");

  Cmd *cmd = _drv.cmd_create();
  if (!cmd)
    return -L4_EBUSY;

  try
    {
      Mmc::Reg_ecsd::Ec32_flush_cache fc(0);
      fc.flush() = 1;
      exec_mmc_switch(cmd, fc.index(), fc.raw);
      cmd->check_error("CMD6: SWITCH/FLUSH_CACHE");
      cmd->work_done();
      cmd->destruct();
    }
  catch (L4::Runtime_error const &e)
    {
      warn.printf("flush fails: %s: %s.\n", e.str(), e.extra_str());
      return -L4_EINVAL;
    }

  cb(L4_EOK, 0); // What to pass for 'size'?

  return L4_EOK;
}

template <class Driver>
int
Device<Driver>::discard(l4_uint64_t offset, Block_device::Inout_block const &block,
                Block_device::Inout_callback const &cb, bool discard)
{
  /*
   * For all blocks:
   *  - Cmd35_tag_erase_group_start first bytes/sector (_addr_mult)
   *  - Cmd36_tag_erase_group_end last byte/sector (_addr_mult)
   *  - Cmd38_erase: arg: 0=erase, 1=trim, 3=discard.
   *  - Cmd13_send_status
   *    - command error: return error
   *    - status.ready_for_data = 0: return error card busy
   *    - status.current_state != Transfer: return error
   */
  (void)offset;
  (void)block;
  (void)cb;
  (void)discard;
  warn.printf("\033[31;1mdiscard\033[m\n");
  return -L4_EINVAL;
}

template <class Driver>
void
Device<Driver>::start_device_scan(Errand::Callback const &cb)
{
  _init_time = Util::read_tsc();

  _drv.init();

  Cmd *cmd = _drv.cmd_create();
  if (!cmd)
    return;

  _drv.set_clock_and_timing(400 * KHz, Mmc::Legacy);

  reset_sdio(cmd);

  _init_thread = std::thread([this, cmd, cb]
    {
      struct Wakeup_handler : public L4::Irqep_t<Wakeup_handler>
      {
        long handle_irq()
        { return 0; }
      };
      auto wakeup = std::make_shared<Wakeup_handler>();

      bool failed = false;
      try
        {
          auto me = Pthread::L4::cap(pthread_self());

          // During initialization receive IRQ directly  (receive_irq())
          L4Re::chksys(l4_error(_irq->bind_thread(me, 0)),
                       "Bind IRQ to initialization thread.");

          _drv.set_voltage(Mmc::Voltage_330);
          // Delay required after changing voltage.
          _drv.delay(Voltage_delay_ms);
          _drv.set_bus_width(Mmc::Bus_width::Width_1bit);
          _drv.set_clock_and_timing(400 * KHz, Mmc::Legacy);

          for (unsigned i = 0; i < 2; ++i)
            {
              cmd->init(Mmc::Cmd0_go_idle_state);
              cmd_exec(cmd);
              cmd->check_error("CMD0: GO_IDLE");

              Mmc::Arg_cmd8_send_if_cond a8;
              a8.check_pattern() = 0xaa;
              a8.voltage_suppl() = a8.Volt_27_36;
              cmd->init_arg(Mmc::Cmd8_send_if_cond, a8.raw);
              cmd->flags.expected_error() = true;
              cmd_exec(cmd);

              if (   cmd->status == Cmd::Success
                  || cmd->status == Cmd::Cmd_timeout)
                {
                  info.printf("Initial SEND_IF_COND response: %08x (voltage %saccepted).\n",
                              Mmc::Rsp_r7(cmd->resp[0]).raw,
                              Mmc::Rsp_r7(cmd->resp[0]).voltage_accepted()
                              ? "" : "NOT ");
                  break;
                }
              else if (i > 0)
                L4Re::throw_error(-L4_EIO, "Unusable card");
            }

          if (   !power_up_sd(cmd)
              && !power_up_mmc(cmd))
            L4Re::throw_error(-L4_EIO, "Neither SD nor eMMC.");

          info.printf("DMA mode:%s, cmd23:%s, auto cmd23:%s.\n",
                      _drv.dma_adma2() ? "adma2" : "sdma",
                      yes_no(_has_cmd23), yes_no(_drv.auto_cmd23()));

          cmd->work_done();
          cmd->destruct();
        }
      catch (L4::Runtime_error const &e)
        {
          _drv.dump();
          warn.printf("%s: %s. Skipping.\n", e.str(), e.extra_str());
          failed = true;
        }

      // Initialization done: No longer use receive_irq().
      _irq->detach();
      L4Re::chksys(l4_error(_icu->unbind(_irq_num, _irq)),
                   "Unbind IRQ after initialization.");

      // Register wakeup object -- see below.
      _registry->register_irq_obj(wakeup.get());

      // Schedule an immediate errand. But currently the server loop is most
      // likely waiting for requests. Well, we could also move the following
      // code to wakeup::handle_irq() but in that case, joining the init thread
      // gets difficult as wakeup is stored in the context of the init thread.
      Errand::schedule([this, cb, failed, wakeup]
        {
          _registry->unregister_obj(wakeup.get());
          _init_thread.join();

          if (!failed)
            {
              // From now on, the server loop handles the interrupt.
              _irq = L4Re::chkcap(_registry->register_irq_obj(this),
                                  "Register IRQ server object.");

              L4Re::chksys(_icu->set_mode(_irq_num, _irq_mode), "Set IRQ mode.");

              int ret = L4Re::chksys(l4_error(_icu->bind(_irq_num, _irq)),
                                     "Bind interrupt to ICU.");
              _irq_unmask_at_icu = ret == 1;

              cb();
            }
        }, 0);

      // Wakeup the server loop.
      wakeup->obj_cap()->trigger();
    });
}

/**
 * Explicitly receive a single device IRQ.
 *
 * Actually this function is not used with the asynchronous handling.
 */
template <class Driver>
void
Device<Driver>::receive_irq(bool is_data) const
{
  struct Timeout
  {
    Timeout(l4_uint32_t timeout)
    { l4_rcv_timeout(l4_timeout_from_us(timeout), &_timeout); }
    l4_timeout_t timeout() const { return _timeout; }
    l4_timeout_t _timeout = L4_IPC_NEVER_INITIALIZER;
  };
  static Timeout timeout(Timeout_irq_us);
  L4Re::chksys(l4_ipc_error(_irq->receive(timeout.timeout()), l4_utcb()),
               "Receive IRQ.");

  if (trace.is_active())
    {
      if (is_data)
        _drv.show_interrupt_status("Receive IRQ (data): got ");
      else
        _drv.show_interrupt_status("Receive IRQ (cmd): got ");
    }
}

template <class Driver>
void
Device<Driver>::handle_irq()
{
  if (trace.is_active())
    _drv.show_interrupt_status("HANDLE IRQ: ");
  ++_stat_ints;
  try
    {
      Cmd *cmd = _drv.handle_irq();
      if (cmd)
        {
          if (cmd->progress())
            {
              // Command not yet finished -- get ready for further interrupts.
              unmask_interrupt();
              return;
            }

          // Special handling for in/out commands.
          if (cmd->flags.inout())
            {
              handle_irq_inout(cmd);
              return;
            }

          cmd_queue_kick();
        }
    }
  catch (L4::Runtime_error const &e)
    {
      warn.printf("Exception triggered: %s: %s\n", e.str(), e.extra_str());
    }
}

template <class Driver>
void
Device<Driver>::handle_irq_inout(Cmd *cmd)
{
  if (!cmd->cb_io)
    L4Re::throw_error(-L4_EINVAL, "No context for async command");

  Work_status work;
  if (cmd->flags.inout_cmd12() && !_drv.auto_cmd12())
    {
      // Send CMD12 if not done automatically by the controller.
      cmd->flags.inout_cmd12() = 0;
      cmd->status_cmd12 = cmd->status; // remember the transfer status
      cmd->reinit_inout_nodata(cmd->flags.inout_read()
                                 ? Mmc::Cmd12_stop_transmission_rd
                                 : Mmc::Cmd12_stop_transmission_wr, 0);
      work = More_work;
    }
  else
    {
      if (   cmd->cmd == Mmc::Cmd12_stop_transmission_rd
          || cmd->cmd == Mmc::Cmd12_stop_transmission_wr)
        cmd->status = cmd->status_cmd12; // restore the transfer status
      if (cmd->error())
        {
          l4_uint64_t transferred = bytes_transferred(cmd);
          info.printf("\033[31mInout error (%s): %lld bytes transferred.\033[m\n",
                      cmd->str_error(), transferred);
          cmd->cb_io(-L4_EIO, transferred);
          work = Work_done;
        }
      else
        {
          if (_drv.dma_adma2())
            work = handle_irq_inout_adma2(cmd);
          else
            work = handle_irq_inout_sdma(cmd);
        }
    }

  if (work == Work_done)
    {
      cmd->work_done();
      cmd->destruct();
    }

  cmd_queue_kick();
}

template <class Driver>
typename Device<Driver>::Work_status
Device<Driver>::handle_irq_inout_sdma(Cmd *cmd)
{
  if (cmd->status == Cmd::Success)
    {
      // Read/Write command finished successfully, go to next block.
      if (cmd->cmd != Mmc::Cmd23_set_block_count)
        {
          auto const *b = cmd->blocks;
          cmd->sector  += b->num_sectors;
          cmd->sectors += b->num_sectors;
          do
            {
              b = b->next.get();
            }
          while (b && b->num_sectors == 0);
          cmd->blocks = b;
        }
    }

  return transfer_block_sdma(cmd);
}

template <class Driver>
typename Device<Driver>::Work_status
Device<Driver>::transfer_block_sdma(Cmd *cmd)
{
  auto const *b = cmd->blocks;
  if (!b)
    {
      cmd->cb_io(L4_EOK, bytes_transferred(cmd));
      return Work_done;
    }

  if (b->num_sectors == 1)
    {
      cmd->reinit_inout_data(cmd->flags.inout_read()
                               ? Mmc::Cmd17_read_single_block
                               : Mmc::Cmd24_write_block,
                             cmd->sector * _addr_mult, 1, 512,
                             Cmd::Flag_auto_cmd23::No_auto_cmd23);
    }
  else if (_has_cmd23 && cmd->cmd != Mmc::Cmd23_set_block_count)
    {
      // Previous command was either transfer command or CMD12.
      Mmc::Arg_cmd23_set_block_count a23;
      a23.blocks() = b->num_sectors;
      cmd->reinit_inout_nodata(Mmc::Cmd23_set_block_count, a23.raw);
    }
  else
    {
      cmd->reinit_inout_data(cmd->flags.inout_read()
                               ? Mmc::Cmd18_read_multiple_block
                               : Mmc::Cmd25_write_multiple_block,
                             cmd->sector * _addr_mult, b->num_sectors, 512,
                             Cmd::Flag_auto_cmd23::No_auto_cmd23);
      if (!_has_cmd23)
        cmd->flags.inout_cmd12() = 1;
    }

  return More_work;
}

template <class Driver>
void
Device<Driver>::set_block_count_adma2(Cmd *cmd)
{
  l4_uint32_t sectors = 0;
  for (auto const *b = cmd->blocks; b; b = b->next.get())
    sectors += b->num_sectors;

  trace2.printf("set_block_count_adma2: sector=%u sectors=%u\n",
                cmd->sector, sectors);

  if (!_has_cmd23 || _drv.auto_cmd23())
    {
      cmd->reinit_inout_data(cmd->flags.inout_read()
                               ? Mmc::Cmd18_read_multiple_block
                               : Mmc::Cmd25_write_multiple_block,
                             cmd->sector * _addr_mult, sectors, 512,
                             _has_cmd23 && _drv.auto_cmd23()
                               ? Cmd::Flag_auto_cmd23::Do_auto_cmd23
                               : Cmd::Flag_auto_cmd23::No_auto_cmd23);
      if (!_has_cmd23)
        cmd->flags.inout_cmd12() = 1;
    }
  else
    {
      Mmc::Arg_cmd23_set_block_count a23;
      a23.blocks() = sectors;
      cmd->blockcnt = sectors;
      cmd->reinit_inout_nodata(Mmc::Cmd23_set_block_count, a23.raw);
    }
}

template <class Driver>
typename Device<Driver>::Work_status
Device<Driver>::handle_irq_inout_adma2(Cmd *cmd)
{
  // This function is only called once or twice:
  //  1. With Auto CMD23, this function is called once to finish the transfer.
  //  2. Without Auto CMD23, the previous command didn't transfer data (hence
  //     was CMD23), so now send the actual transfer command.
  // In the latter case, mark inout_cmd12 in case CMD23 isn't available.
  if (cmd->cmd != Mmc::Cmd23_set_block_count)
    {
      // Previous command was either transfer command or CMD12.
      cmd->cb_io(L4_EOK, bytes_transferred(cmd));
      return Work_done;
    }
  else
    {
      cmd->reinit_inout_data(cmd->flags.inout_read()
                             ? Mmc::Cmd18_read_multiple_block
                             : Mmc::Cmd25_write_multiple_block,
                             cmd->sector * _addr_mult, cmd->blockcnt, 512,
                             Cmd::Flag_auto_cmd23::No_auto_cmd23);
      if (!_has_cmd23)
        cmd->flags.inout_cmd12() = 1;
      return More_work;
    }
}

template <class Driver>
void
Device<Driver>::unmask_interrupt() const
{
  if (_irq_unmask_at_icu)
    _icu->unmask(_irq_num);
  else
    L4::Irqep_t<Device<Driver>>::obj_cap()->unmask();
}

template <class Driver>
void
Device<Driver>::mmc_set_bus_width(Cmd *cmd,
                                  Mmc::Reg_ecsd::Ec183_bus_width::Width width,
                                  bool strobe)
{
  Mmc::Reg_ecsd::Ec183_bus_width bw(0);
  bw.bus_mode_select() = width;
  bw.enhanced_strobe() = strobe;
  exec_mmc_switch(cmd, bw.index(), bw.raw);
  if (cmd->error())
    warn.printf("Set bus width (%s) failed.\n", bw.str_bus_width());
  else if (cmd->switch_error())
    warn.printf("Set bus width (%s) failed (status %08x).\n",
                bw.str_bus_width(), cmd->mmc_status().raw);
  else
    _drv.set_bus_width(Mmc::Bus_width::Width_8bit);
}

template <class Driver>
void
Device<Driver>::mmc_set_timing(Cmd *cmd,
                       Mmc::Reg_ecsd::Ec185_hs_timing::Timing hs_timing,
                       Mmc::Timing mmc_timing, l4_uint32_t freq, bool strobe)
{
  Mmc::Reg_ecsd::Ec185_hs_timing ht(0);
  ht.timing_interface() = hs_timing;
  exec_mmc_switch(cmd, ht.index(), ht.raw);

  // eMMC spec 6.6.2.2
  if (cmd->error() || cmd->switch_error())
    {
      warn.printf("Switch '%s' timing failed (%s).\n",
                  Mmc::str_timing(mmc_timing), cmd->str_status().c_str());
      cmd->status = Cmd::Error;
      return;
    }

  _drv.set_clock_and_timing(freq, mmc_timing, strobe);
  cmd->init_arg(Mmc::Cmd13_send_status, _rca << 16);
  cmd_exec(cmd);
  if (cmd->error() || cmd->switch_error())
    {
      warn.printf("Set timing (%s, %s) error (%s).\n",
                  Mmc::str_timing(mmc_timing),
                  Util::readable_freq(freq).c_str(),
                  cmd->str_status().c_str());
      cmd->status = Cmd::Error;
      return;
    }

  cmd->init_data(Mmc::Cmd8_send_ext_csd, 0, 512, _io_buf.pget(), 0);
  cmd_exec(cmd);
  if (cmd->error())
    {
      warn.printf("Set timing (%s, %s) error (CMD8: %s).\n",
                  Mmc::str_timing(mmc_timing),
                  Util::readable_freq(freq).c_str(),
                  cmd->str_status().c_str());
      cmd->status = Cmd::Error;
      return;
    }

  unsigned t = _ecsd.ec185_hs_timing.timing_interface();
  if (t != hs_timing)
    {
      warn.printf("Set timing (%s) failed (timing %d/%d).\n",
                  Util::readable_freq(freq).c_str(),
                  t, hs_timing);
      cmd->status = Cmd::Error;
      return;
    }
}

template <class Driver>
void
Device<Driver>::adapt_ocr(Mmc::Reg_ocr ocr_dev, Mmc::Arg_acmd41_sd_send_op *a41)
{
  Mmc::Reg_ocr ocr_drv = _drv.supported_voltage();
  Mmc::Arg_acmd41_sd_send_op arg;
  arg.mv3500_3600() = ocr_dev.mv3500_3600() && ocr_drv.mv3500_3600();
  arg.mv3400_3500() = ocr_dev.mv3400_3500() && ocr_drv.mv3400_3500();
  arg.mv3300_3400() = ocr_dev.mv3300_3400() && ocr_drv.mv3300_3400();
  arg.mv3200_3300() = ocr_dev.mv3200_3300() && ocr_drv.mv3200_3300();
  arg.mv3100_3200() = ocr_dev.mv3100_3200() && ocr_drv.mv3100_3200();
  arg.mv3000_3100() = ocr_dev.mv3000_3100() && ocr_drv.mv3000_3100();
  arg.mv2900_3000() = ocr_dev.mv2900_3000() && ocr_drv.mv2900_3000();
  arg.mv2800_2900() = ocr_dev.mv2800_2900() && ocr_drv.mv2800_2900();
  arg.mv2700_2800() = ocr_dev.mv2700_2800() && ocr_drv.mv2700_2800();
  arg.hcs() = 1; // Host supports high-capacity, should only be set on >= SD2.0
  *a41 = arg;
}

template <class Driver>
void
Device<Driver>::reset_sdio(Cmd *cmd)
{
  info.printf("Resetting sdio...\n");

  const int Sdio_cccr_abort = 0x6; // I/O card reset
  Mmc::Arg_cmd52_io_rw_direct a52;
  a52.address() = Sdio_cccr_abort;
  a52.function() = 0;
  a52.write() = 0;
  cmd->init_arg(Mmc::Cmd52_io_rw_direct, a52.raw);
  cmd->flags.expected_error() = true;
  cmd_exec(cmd);
  if (!cmd->error())
    L4Re::throw_error(-L4_EIO, "IO_RW_DIRECT (read) succeeded");

  a52.raw = 0;
  a52.write_data() = 0x8;
  a52.address() = Sdio_cccr_abort;
  a52.function() = 0;
  a52.write() = 1;

  cmd->init_arg(Mmc::Cmd52_io_rw_direct, a52.raw);
  cmd->flags.expected_error() = true;
  cmd_exec(cmd);
}

template <class Driver>
bool
Device<Driver>::power_up_sd(Cmd *cmd)
{
  info.printf("Trying sd...\n");

  _rca = 0;

  // The following command would detect an SDIO card and it would also enable
  // the IO part of the SDIO. Actually we assume that an SDIO card with SD
  // functions does not provide the SDIO interface by default.
  if (0)
    {
      cmd->init(Mmc::Cmd5_io_send_op_cond);
      cmd->flags.expected_error() = true;
      cmd_exec(cmd);
      if (!cmd->error())
        {
          // SDIO spec 3.0 / section 3.3:
          // Actually an SD memory only card may respond to CMD5. The proper
          // response for a memory only card would be Memory Present = 1 and
          // Number of I/O Functions = 0.
          Mmc::Rsp_r4 rsp{cmd->resp[0]};
          if (rsp.mem_pres() != 1 || rsp.num_io() != 0)
            {
              info.printf("SDIO card detected (R4=%08x)!\n", rsp.raw);
              L4Re::throw_error(-L4_EINVAL, "IO_SEND_OP_COND succeeded");
            }
        }
    }

  // Get SD card's operating conditions.
  mmc_app_cmd(cmd, Mmc::Acmd41_sd_app_op_cond, 0);
  if (cmd->error())
    {
      info.printf("SD_APP_OP_COND failed (%s)\n", cmd->str_error());
      return false;
    }

  warn.printf("Found SD card version 2 or later, OCR=%08x.\n", cmd->resp[0]);

  // SD Host Controller Simplified Specification Figure 3-6

  Mmc::Arg_acmd41_sd_send_op a41;
  adapt_ocr(Mmc::Reg_ocr(cmd->resp[0]), &a41);

  if (_drv.supp_uhs_timings(Mmc::Uhs_modes))
    {
      a41.s18r() = 1;
      if (_drv.xpc_supported(Mmc::Voltage_180))
        a41.xpc() = 1;
    }
  else
    {
      if (_drv.xpc_supported(Mmc::Voltage_330))
        a41.xpc() = 1;
    }

  // SD spec physical layer simplified spec 8.00 / 4.2.4.1
  bool v18 = false;
  for (unsigned i = 0; i < 10; ++i)
    {
      _drv.set_bus_width(Mmc::Bus_width::Width_1bit);
      _drv.set_clock_and_timing(400 * KHz, Mmc::Legacy);

      cmd->init(Mmc::Cmd0_go_idle_state);
      cmd_exec(cmd);
      cmd->check_error("CMD0: GO_IDLE");

      Mmc::Arg_cmd8_send_if_cond a8;
      a8.check_pattern() = 0xaa;
      a8.voltage_suppl() = a8.Volt_27_36;
      cmd->init_arg(Mmc::Cmd8_send_if_cond, a8.raw);
      cmd_exec(cmd);
      cmd->check_error("CMD8: SEND_IF_COND");
      Mmc::Rsp_r7 r7(cmd->resp[0]);
      trace.printf("SEND_IF_COND response: %08x.\n", r7.raw);

      // XXX if this fails then SDSC version 1.01 or version 1.10

      for (unsigned j = 0; j < 100; ++j)
        {
          mmc_app_cmd(cmd, Mmc::Acmd41_sd_app_op_cond, a41.raw);
          cmd->check_error("ACMD41: SD_APP_OP_COND-2");
          trace.printf("Got OCR=%08x\n", cmd->resp[0]);
          if (Mmc::Reg_ocr(cmd->resp[0]).not_busy())
            break;

          _drv.delay(5);
        }

      Mmc::Reg_ocr ocr(cmd->resp[0]);
      if (!ocr.not_busy())
        L4Re::throw_error(-L4_EINVAL, "Card still busy");

      warn.printf("Resulting OCR after SD_APP_OP_COND: %08x\n", ocr.raw);

      _addr_mult = ocr.ccs() ? 1 : sector_size();

      if (ocr.ccs() && ocr.s18a())
        {
          l4_uint64_t now = Util::read_tsc();

          // SDHCI Controller spec 4.20 / 3.6.1:
          cmd->init_arg(Mmc::Cmd11_voltage_switch, 0);
          cmd_exec(cmd);
          cmd->check_error("CMD11: VOLTAGE_SWITCH");
          if (cmd->mmc_status().error())
            L4Re::throw_error(-L4_EINVAL, "CMD11 status error");

          if (!_drv.card_busy())
            {
              trace.printf("card not busy, retry\n");
              continue; // XXX doesn't work -- need a complete re-init!
            }

          _drv.delay(2);
          _drv.set_clock_and_timing(0, Mmc::Legacy);
          _drv.set_voltage(Mmc::Voltage_180);
          _drv.delay(5);
          _drv.set_clock_and_timing(400 * KHz, Mmc::Uhs_sdr12);
          _drv.delay(5);
          if (_drv.card_busy())
            L4Re::throw_error(-L4_EINVAL, "Still busy after set voltage");

          trace.printf("Power switch to 1.8V took %llums.\n",
                       Util::tsc_to_ms(Util::read_tsc() - now));
          v18 = true;
          _drv.delay(5);
        }
      else
        warn.printf("\033[31mCard does not announce support for 1.8V.\033[m\n");

      break;
    }

  // *** Initialize SD card ***

  cmd->init(Mmc::Cmd2_all_send_cid);
  cmd_exec(cmd);
  cmd->check_error("CMD2: ALL_SEND_CID");

  Mmc::Reg_cid cid(cmd->resp);
  info.printf("product: '%s', manufactured %d/%d, mid=%02x, psn=%08x\n",
              readable_product(cid.sd.pnm()).c_str(), cid.sd.mmth(),
              cid.sd.myr(), cid.sd.mid().get(), cid.sd.psn());

  // Use the PSN as identifier for the whole device. The method `match_hid()`
  // will match for this string.
  snprintf(_hid, sizeof(_hid), "%08x", cid.sd.psn());

  cmd->init_arg(Mmc::Cmd3_send_relative_addr, 0);
  cmd_exec(cmd);
  cmd->check_error("CMD3: SEND_RELATIVE_ADDR");

  _rca = cmd->resp[0] >> 16;

  cmd->init_arg(Mmc::Cmd9_send_csd, _rca << 16);
  cmd_exec(cmd);
  cmd->check_error("CMD9: SEND_CSD");

  Mmc::Reg_csd csd(cmd->resp);
  show_csd(csd);

  cmd->init_arg(Mmc::Cmd7_select_card, _rca << 16);
  cmd_exec(cmd);
  cmd->check_error("CMD7: SELECT_CARD");

  if (cmd->mmc_status().device_is_locked())
    {
      // Execute CMD42 to unlock the device providing a password.
      L4Re::throw_error(-L4_EIO, "Device is locked!");
    }

  mmc_app_cmd(cmd, Mmc::Acmd51_send_scr, 0, 8, _io_buf.pget(),
              reinterpret_cast<l4_addr_t>(_io_buf.get<void>()));
  cmd->check_error("ACMD51: SEND_SCR");

  if (false && trace.is_active())
    _io_buf.dump("Got SCR:", 4, 8);

  Mmc::Reg_scr const scr(_io_buf.get<l4_uint8_t const>());
  info.printf("SCR version %s, 1-bit bus:%s, 4-bit bus:%s, cmd23:%s\n",
              scr.sd_spec_str(), yes_no(scr.sd_bus_width_1()),
              yes_no(scr.sd_bus_width_4()), yes_no(scr.cmd23_support()));

  if (scr.sd_spec_vers() < 300)
    L4Re::throw_error(-L4_EINVAL, "SD spec < 3.0, adapt implementation");

  _has_cmd23 = scr.cmd23_support();

  bool has_bus_4bit = scr.sd_bus_width_4();

  mmc_app_cmd(cmd, Mmc::Acmd13_sd_status, 0, 64, _io_buf.pget(),
              reinterpret_cast<l4_addr_t>(_io_buf.get<void>()));
  cmd->check_error("ACMD13: SD_STATUS");

  if (false && trace.is_active())
    _io_buf.dump("Got SSR:", 4, 64);

  Mmc::Reg_ssr const ssr(_io_buf.get<l4_uint8_t const>());
  info.printf("SSR: speed:'%s', UHS_speed:'%s', AU size:%s, cc:%u\n",
              ssr.str_speed_class(), ssr.str_uhs_speed_grade(),
              Util::readable_size(1U << (12 + ssr.au_size())).c_str(),
              ssr.supp_cmd_queue().get());

  Mmc::Arg_cmd6_switch_func a6;
  a6.grp1_acc_mode() = a6.Grp1_sdr12;
  a6.mode() = a6.Check_function;
  cmd->init_data(Mmc::Cmd6_switch_func, a6.raw, 64, _io_buf.pget(),
                 reinterpret_cast<l4_addr_t>(_io_buf.get<void>()));
  cmd_exec(cmd);
  cmd->check_error("CMD6: SWITCH_FUNC/GET");

  if (false && trace.is_active())
    _io_buf.dump("Got switch function status:", 4, 64);

  Mmc::Reg_switch_func sf(_io_buf.get<l4_uint8_t>());
  info.printf("access: sdr12:%s, sdr25:%s, sdr50:%s, sdr104:%s, ddr50:%s\n",
              yes_no(sf.acc_mode_sdr12()), yes_no(sf.acc_mode_sdr25()),
              yes_no(sf.acc_mode_sdr50()), yes_no(sf.acc_mode_sdr104()),
              yes_no(sf.acc_mode_ddr50()));

  trace.printf("power limit: 0.72W:%s, 1.44W:%s, 2.16W:%s, 2.88W:%s, 1.80W:%s\n",
               yes_no(sf.power_limit_072w()), yes_no(sf.power_limit_144w()),
               yes_no(sf.power_limit_216w()), yes_no(sf.power_limit_288w()),
               yes_no(sf.power_limit_180w()));

  Mmc::Timing mmc_timing;
  Mmc::Arg_cmd6_switch_func::Grp1_acc_mode a6_timing;
  l4_uint32_t freq;
  if (!v18 && (sf.acc_mode_sdr104() || sf.acc_mode_ddr50() || sf.acc_mode_sdr50()))
    {
      // See Physical Layer Simplified Specification Version 8.00 / 4.3.10.3:
      // It may happen that a card has already been switched into in 1.8V mode.
      warn.printf("\033[31mCard apparently already in 1.8V mode.\033[m\n");

      _drv.delay(2);
      _drv.set_clock_and_timing(0, Mmc::Legacy);
      _drv.set_voltage(Mmc::Voltage_180);
      _drv.delay(5);
      _drv.set_clock_and_timing(400 * KHz, Mmc::Uhs_sdr12);
      _drv.delay(5);
      if (_drv.card_busy())
        L4Re::throw_error(-L4_EINVAL, "Still busy after set voltage");
      v18 = true;
      _drv.delay(5);
    }

  // Start with the fastest supported mode. On tuning failure, try the next
  // slower supported mode by masking modes using _device_type_disable.
  for (;;)
    {
      if (v18 && _drv.supp_uhs_timings(Mmc::Uhs_sdr104) && sf.acc_mode_sdr104()
          && !(_device_type_disable.sd & Mmc::Uhs_sdr104))
        {
          mmc_timing = Mmc::Uhs_sdr104;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_sdr104;
          freq = 200 * MHz;
        }
      else if (v18 && _drv.supp_uhs_timings(Mmc::Uhs_ddr50) && sf.acc_mode_ddr50()
               && !(_device_type_disable.sd & Mmc::Uhs_ddr50))
        {
          mmc_timing = Mmc::Uhs_ddr50;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_ddr50;
          freq = 50 * MHz;
        }
      else if (v18 && _drv.supp_uhs_timings(Mmc::Uhs_sdr50) && sf.acc_mode_sdr50()
               && !(_device_type_disable.sd & Mmc::Uhs_sdr50))
        {
          mmc_timing = Mmc::Uhs_sdr50;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_sdr50;
          freq = 100 * MHz;
        }
      else if (_drv.supp_uhs_timings(Mmc::Uhs_sdr25) && sf.acc_mode_sdr25()
               && !(_device_type_disable.sd & Mmc::Uhs_sdr25))
        {
          mmc_timing = Mmc::Uhs_sdr25;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_sdr25;
          freq = 50 * MHz;
        }
      else if (_drv.supp_uhs_timings(Mmc::Uhs_sdr12) && sf.acc_mode_sdr12()
               && !(_device_type_disable.sd & Mmc::Uhs_sdr12))
        {
          mmc_timing = Mmc::Uhs_sdr12;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_sdr12;
          freq = 25 * MHz;
        }
      else
        {
          mmc_timing = Mmc::Hs;
          a6_timing = Mmc::Arg_cmd6_switch_func::Grp1_sdr12;
          freq = 25 * MHz;
        }

      // Bus width -- also for HS!
      if (has_bus_4bit)
        {
          Mmc::Arg_acmd6_sd_set_bus_width a6;
          a6.bus_width() = a6.Bus_width_4bit;
          mmc_app_cmd(cmd, Mmc::Acmd6_set_bus_width, a6.raw);
          cmd->check_error("ACMD6: SET_BUS_WIDTH");

          _drv.set_bus_width(Mmc::Width_4bit);
        }

      if (mmc_timing != Mmc::Hs)
        {
          Mmc::Arg_cmd6_switch_func::Grp4_power_limit a6_power;
          if (_drv.supp_power_limit(Mmc::Power_288w) && sf.power_limit_288w())
            a6_power = Mmc::Arg_cmd6_switch_func::Grp4_288w;
          else if (_drv.supp_power_limit(Mmc::Power_216w) && sf.power_limit_216w())
            a6_power = Mmc::Arg_cmd6_switch_func::Grp4_216w;
          else if (_drv.supp_power_limit(Mmc::Power_180w) && sf.power_limit_180w())
            a6_power = Mmc::Arg_cmd6_switch_func::Grp4_180w;
          else if (_drv.supp_power_limit(Mmc::Power_144w) && sf.power_limit_144w())
            a6_power = Mmc::Arg_cmd6_switch_func::Grp4_144w;
          else
            a6_power = Mmc::Arg_cmd6_switch_func::Grp4_default;

          // Allowed power consumption.
          if (a6_power != Mmc::Arg_cmd6_switch_func::Grp4_default)
            {
              a6.reset();
              a6.grp4_power_limit() = a6_power;
              a6.mode() = a6.Set_function;
              cmd->init_data(Mmc::Cmd6_switch_func, a6.raw, 64, _io_buf.pget(),
                             reinterpret_cast<l4_addr_t>(_io_buf.get<void>()));
              cmd_exec(cmd);
              cmd->check_error("CMD6: SWITCH_FUCN/SET_POWER");
              if (sf.fun_sel_grp4() == sf.Invalid_function)
                L4Re::throw_error(-L4_EINVAL, "Invalid function trying to set power");
            }
        }

      a6.reset();
      a6.grp1_acc_mode() = a6_timing;
      a6.mode() = a6.Set_function;
      cmd->init_data(Mmc::Cmd6_switch_func, a6.raw, 64, _io_buf.pget(),
                     reinterpret_cast<l4_addr_t>(_io_buf.get<void>()));
      cmd_exec(cmd);
      cmd->check_error("CMD6: SWITCH_FUNC/SET_MODE");
      if (sf.fun_sel_grp1() == sf.Invalid_function)
        L4Re::throw_error(-L4_EINVAL, "Invalid function trying to set mode");

      _drv.set_clock_and_timing(freq, mmc_timing);

      // Tuning: SDR104: Always. SDR50: Only if controller demands.
      if (   mmc_timing == Mmc::Uhs_sdr104
          || mmc_timing == Mmc::Uhs_ddr50
          || (mmc_timing == Mmc::Uhs_sdr50 && _drv.needs_tuning_sdr50()))
        {
          info.printf("Mode '%s' needs tuning...\n", Mmc::str_timing(mmc_timing));
          _drv.reset_tuning();
          bool success = false;
          unsigned i;
          for (i = 0; i < Mmc::Arg_cmd19_send_tuning_block::Max_loops; ++i)
            {
              cmd->init(Mmc::Cmd19_send_tuning_block);
              cmd_exec(cmd);
              if (cmd->status == Cmd::Success)
                {
                  if (_drv.tuning_finished(&success))
                    break;
                }
              else if (cmd->status == Cmd::Cmd_timeout)
                break;
            }
          if (!success)
            {
              _device_type_disable.sd |= mmc_timing;
              info.printf("\033[31mTuning for mode '%s' failed!\033[m\n",
                          Mmc::str_timing(mmc_timing));
              _drv.set_clock_and_timing(400 * KHz, Mmc::Legacy);
              // Seems this doesn't work. Need to reset more state?
              continue;
            }

          _drv.enable_auto_tuning();
          info.printf("Tuning success.\n");
        }

      break;
    }

  _sd_timing = mmc_timing;

  warn.printf("Device initialization took %llums (%llums busy wait, %llums sleep).\n",
              Util::tsc_to_ms(Util::read_tsc() - _init_time),
              Util::tsc_to_ms(_drv._time_busy),
              Util::tsc_to_ms(_drv._time_sleep));

  warn.printf("\033[33;1mSuccessfully set '%s' timing.\033[m\n",
              Mmc::str_timing(_sd_timing));

  _type = T_sd; // SD card version 2 or later
  return true;
}

template <class Driver>
bool
Device<Driver>::power_up_mmc(Cmd *cmd)
{
  info.printf("Trying mmc...\n");

  cmd->init(Mmc::Cmd1_send_op_cond);
  cmd_exec(cmd);
  if (cmd->error())
    return false;

  warn.printf("Found eMMC device.\n");

  // Only probe
  Mmc::Reg_ocr ocr(cmd->resp[0]);
  trace.printf("OCR: busy=%d voltrange=%05x, ccs=%s, raw=%08x\n",
               !ocr.not_busy(), ocr.voltrange_mmc().get(),
               !!ocr.ccs() ? "sector" : "byte", ocr.raw);

  cmd->init(Mmc::Cmd0_go_idle_state);
  cmd_exec(cmd);
  cmd->check_error("CMD0: GO_IDLE");

  // Not documented but we shall pass the correct OCR as argument to CMD1.
  Mmc::Arg_acmd41_sd_send_op a41;
  adapt_ocr(ocr, &a41);

  for (unsigned i = 0; i < 100; ++i)
    {
      cmd->init_arg(Mmc::Cmd1_send_op_cond, a41.raw);
      cmd_exec(cmd);
      cmd->check_error("CMD1: SEND_OP_COND");

      ocr.raw = cmd->resp[0];
      // see eMMC spec 6.4.2
      trace.printf("OCR: busy=%d voltrange=%05x, ccs=%s, raw=%08x\n",
                   !ocr.not_busy(), ocr.voltrange_mmc().get(),
                   !!ocr.ccs() ? "sector" : "byte", ocr.raw);
      if (ocr.raw != 0x00ff8080 && ocr.raw != 0x40ff8080 && ocr.not_busy())
        break;

      _drv.delay(5);
    }

  ocr.raw = cmd->resp[0];
  if (!ocr.not_busy())
    L4Re::throw_error(-L4_EIO, "Device still busy.");

  _addr_mult = ocr.ccs() ? 1 : sector_size();

  for (unsigned i = 0; i < 5; ++i)
    {
      cmd->init(Mmc::Cmd2_all_send_cid);
      cmd_exec(cmd);
      if (cmd->status == Cmd::Success)
        break;
    }

  cmd->check_error("CMD2: ALL_SEND_CID");

  Mmc::Reg_cid cid(cmd->resp);
  info.printf("product: '%s', manufactured %d/%d, mid = %02x, psn = %08x\n",
              readable_product(cid.mmc.pnm()).c_str(), cid.mmc.mmth(),
              cid.mmc.myr(), cid.mmc.mid().get(), cid.mmc.psn());

  // Use the PSN as identifier for the whole device. The method `match_hid()`
  // will match for this string.
  snprintf(_hid, sizeof(_hid), "%08x", cid.mmc.psn());

  cmd->init_arg(Mmc::Cmd3_set_relative_addr, _rca << 16);
  cmd_exec(cmd);
  cmd->check_error("CMD3: SET_RELATIVE_ADDR");

  cmd->init_arg(Mmc::Cmd9_send_csd, _rca << 16);
  cmd_exec(cmd);
  cmd->check_error("CMD9: SEND_CSD");

  Mmc::Reg_csd csd(cmd->resp);
  show_csd(csd);

  if (csd.s3.spec_vers() < 4)
    {
      // Lacks support for SWITCH, SEND_EXT_CSD, ...
      L4Re::throw_error(-L4_EINVAL, "eMMC device too old.");
    }

  cmd->init_arg(Mmc::Cmd7_select_card, _rca << 16);
  cmd_exec(cmd);
  cmd->check_error("CMD7: SELECT_CARD");

  if (cmd->mmc_status().device_is_locked())
    {
      // Execute CMD42 to unlock the device providing a password.
      L4Re::throw_error(-L4_EIO, "Device is locked!");
    }

  cmd->init_data(Mmc::Cmd8_send_ext_csd, 0, 512, _io_buf.pget(), 0);
  cmd_exec(cmd);
  cmd->check_error("CMD8: SEND_EXT_CSD");

  l4_uint64_t size = 512L * _ecsd.ec212_sec_count;
  if (size)
    trace.printf("Device size (EXT_CSD): %s\n",
                 Util::readable_size(size).c_str());
  else
    trace.printf("No device size reported.\n");

  if (false)
    {
      printf("=== EXT_CSD dump ===\n");
      _io_buf.dump("Got ExtCSD:", 1, 512);
    }

  _mmc_rev = _ecsd.ec192_ext_csd_rev.mmc_rev();
  info.printf("Device rev: 1.%u, eMMC rev: %u.%02u, %s, %s timing.\n",
              _ecsd.ec192_ext_csd_rev.csd_rev(),
              _mmc_rev / 100, _mmc_rev % 100,
              _ecsd.ec183_bus_width.str_bus_width(),
              _ecsd.ec185_hs_timing.str_timing_interface());
  info.printf("Command queuing %ssupported.\n",
              _ecsd.ec308_cmdq_support.cmdq_support() ? "" : "NOT ");
  info.printf("Live time estimation type A: %s, type B: %s.\n",
              Mmc::Reg_ecsd::lifetime_est(_ecsd.ec268_device_life_time_est_typ_a).c_str(),
              Mmc::Reg_ecsd::lifetime_est(_ecsd.ec269_device_life_time_est_typ_b).c_str());

  _num_sectors = _ecsd.ec212_sec_count;
  _size_user   = (l4_uint64_t)_num_sectors << 9;
  _size_boot12 = (l4_uint64_t)_ecsd.ec226_boot_size_mult << 17;
  _size_rpmb   = (l4_uint64_t)_ecsd.ec168_rpmb_size_mult << 17;
  info.printf("Sizes: user: %s, boot1/2: %s, RPMB: %s, active: %s.\n",
              Util::readable_size(_size_user).c_str(),
              Util::readable_size(_size_boot12).c_str(),
              Util::readable_size(_size_rpmb).c_str(),
              _ecsd.ec179_partition_config.str_partition_access());

  _device_type_restricted = _ecsd.ec196_device_type;
  _enh_strobe = _ecsd.ec184_strobe_support;

  // disable certain modes for testing
  _device_type_restricted.disable(_device_type_disable.mmc);

  info.printf("Following device types supported (%02x, strobe=%d):\n",
              _device_type_restricted.raw, _enh_strobe);

  if (!_device_type_restricted.raw)
    info.printf("  None\n");
  else
    for (unsigned i = 0; i < 8; ++i)
      if (_device_type_restricted.raw & (1 << i))
        info.printf("  %s\n", _device_type_restricted.str_device_type(1 << i));

  trace.printf("Driver strength: 4:%s, 3:%s, 2:%s, 1:%s, 0:%s\n",
               yes_na(_ecsd.ec197_driver_strength.type4()),
               yes_na(_ecsd.ec197_driver_strength.type3()),
               yes_na(_ecsd.ec197_driver_strength.type2()),
               yes_na(_ecsd.ec197_driver_strength.type1()),
               yes_na(_ecsd.ec197_driver_strength.type0()));

  Mmc::Reg_ecsd::Ec175_erase_group_def eg(0);
  eg.enable() = 1;
  exec_mmc_switch(cmd, eg.index(), eg.raw);
  cmd->check_error("CMD6: SWITCH/ERASE_GROUP_DEF");

  cmd->init_arg(Mmc::Cmd16_set_blocklen, sector_size());
  cmd_exec(cmd);
  cmd->check_error("CMD16: SET_BLOCK_LENGTH");

  Mmc::Reg_ecsd::Ec34_power_off_notification pon(0);
  pon.notify() = pon.Powered_on;
  exec_mmc_switch(cmd, pon.index(), pon.raw);
  cmd->check_error("CMD6: SWITCH/POWER_OFF_NOTIFICATION");

  Mmc::Reg_ecsd::Ec161_hpi_mgmt hm(0);
  hm.hpi_en() = 0;
  exec_mmc_switch(cmd, hm.index(), hm.raw);
  cmd->check_error("CMD6: SWITCH/HPI_MGMT");

  // Prevent gcc from generating an unaligned 32-bit access to uncached memory!
  l4_uint64_t cache_size_kb
    = ((l4_uint32_t)((l4_uint8_t volatile*)_ecsd.ec249_cache_size)[0])
    + ((l4_uint32_t)((l4_uint8_t volatile*)_ecsd.ec249_cache_size)[1] << 8)
    + ((l4_uint32_t)((l4_uint8_t volatile*)_ecsd.ec249_cache_size)[2] << 16)
    + ((l4_uint32_t)((l4_uint8_t volatile*)_ecsd.ec249_cache_size)[3] << 24);
  if (cache_size_kb)
    {
      info.printf("Device has %s cache -- enabling.\n",
                  Util::readable_size(cache_size_kb << 10).c_str());
      Mmc::Reg_ecsd::Ec33_cache_ctrl cc(0);
      cc.cache_en() = 1;
      exec_mmc_switch(cmd, cc.index(), cc.raw);
      cmd->check_error("CMD6: SWITCH/ENABLE_CACHE");
    }

  Mmc::Reg_ecsd::Ec163_bkops_en bko(0);
  bko.auto_en() = 1;
  exec_mmc_switch(cmd, bko.index(), bko.raw);
  cmd->check_error("CMD6: SWITCH/BKOPS");

  // We don't try to set any 1.2V mode, see below
  _device_type_restricted.disable_12();
  Mmc::Reg_ecsd::Ec196_device_type device_type_test(0);
  _device_type_selected = Mmc::Reg_ecsd::Ec196_device_type::fallback();

  // Start with the fastest supported mode. On tuning failure, try the next
  // slower supported mode by masking modes using _device_type_restricted.
  for (;;)
    {
      _device_type_restricted.raw &= ~device_type_test.raw;
      device_type_test.raw = 0;

      // select settings for (currently) fastest possible mode
      l4_uint32_t freq;
      Mmc::Timing mmc_timing;
      Mmc::Reg_ecsd::Ec185_hs_timing::Timing hs_timing;
      Mmc::Reg_ecsd::Ec183_bus_width::Width bus_width;
      if (_device_type_restricted.hs200_sdr_18())
        {
          freq       = 200 * MHz;
          hs_timing  = Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Hs200;
          mmc_timing = Mmc::Mmc_hs200;
          bus_width  = Mmc::Reg_ecsd::Ec183_bus_width::W_8bit_sdr;
          device_type_test.hs400_ddr_18() = 1;
          device_type_test.hs200_sdr_18() = 1;
        }
      else if (_device_type_restricted.hs52_ddr_18())
        {
          freq       = 52 * MHz;
          hs_timing  = Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Hs;
          mmc_timing = Mmc::Mmc_ddr52;
          bus_width  = Mmc::Reg_ecsd::Ec183_bus_width::W_8bit_ddr;
          device_type_test.hs52_ddr_18() = 1;
        }
      else if (_device_type_restricted.hs52())
        {
          freq       = 52 * MHz;
          hs_timing  = Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Hs;
          mmc_timing = Mmc::Hs;
          bus_width  = Mmc::Reg_ecsd::Ec183_bus_width::W_8bit_sdr;
          device_type_test.hs52() = 1;
        }
      else if (_device_type_restricted.hs26())
        {
          freq       = 26 * MHz;
          hs_timing  = Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Backward_compat;
          mmc_timing = Mmc::Hs;
          bus_width  = Mmc::Reg_ecsd::Ec183_bus_width::W_8bit_sdr;
          device_type_test.hs26() = 1;
        }
      else
        L4Re::throw_error(-L4_EINVAL, "Cannot initialize timing");

      _drv.set_voltage(Mmc::Voltage_180);
      // Delay required after changing voltage.
      _drv.delay(Voltage_delay_ms);

      mmc_set_bus_width(cmd, bus_width);
      if (cmd->error())
        continue;

      // For HS400 with enhanced strobe, don't switch to HS200 first.
      if (!_device_type_restricted.hs400_ddr_18() || !_enh_strobe)
        {
          mmc_set_timing(cmd, hs_timing, mmc_timing, freq);
          if (cmd->error())
            continue;

          if (device_type_test.hs200_sdr_18())
            {
              bool success = false;
              unsigned i;
              for (i = 0; i < Mmc::Arg_cmd21_send_tuning_block::Max_loops; ++i)
                {
                  cmd->init(Mmc::Cmd21_send_tuning_block);
                  cmd_exec(cmd);
                  if (cmd->status == Cmd::Success)
                    {
                      if (_drv.tuning_finished(&success))
                        break;
                    }
                  else if (cmd->status == Cmd::Cmd_timeout)
                    break;
                }
              if (!success)
                continue;
            }
        }

      if (_device_type_restricted.hs400_ddr_18())
        {
          device_type_test.hs200_sdr_18() = 0; // test HS200 separately

          // eMMC spec 6.6.2.3
          mmc_set_timing(cmd, Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Hs,
                         Mmc::Hs, 52 * MHz);
          if (cmd->error())
            continue;

          mmc_set_bus_width(cmd, Mmc::Reg_ecsd::Ec183_bus_width::W_8bit_ddr,
                            _enh_strobe);
          if (cmd->error())
            continue;

          mmc_set_timing(cmd, Mmc::Reg_ecsd::Ec185_hs_timing::Timing::Hs400,
                         Mmc::Mmc_hs400, 200 * MHz, _enh_strobe);
          if (cmd->error())
            continue;
        }

      _device_type_selected = device_type_test;
      break;
    }

  warn.printf("Device initialization took %llums (%llums busy wait, %llums sleep).\n",
              Util::tsc_to_ms(Util::read_tsc() - _init_time),
              Util::tsc_to_ms(_drv._time_busy),
              Util::tsc_to_ms(_drv._time_sleep));
  trace.printf("%u times redo status due to programming state.\n",
               _prg_cnt);
  for (auto it = _prg_map.begin(); it != _prg_map.end(); ++it)
    trace.printf("  switch %d: %d times\n",
                 (unsigned)it->first, (unsigned)it->second);
  auto mode = _device_type_selected.raw;
  warn.printf("\033[33%smSuccessfully set '%s'.\033[m\n",
              _device_type_selected.hs400_ddr_18() ? ";1" : "",
              Mmc::Reg_ecsd::Ec196_device_type::str_device_type(mode));

  _type = T_mmc;
  return true;
}

template <class Driver>
void
Device<Driver>::exec_mmc_switch(Cmd *cmd, l4_uint8_t idx, l4_uint8_t val,
                                bool with_status)
{
  Mmc::Arg_cmd6_switch a6;
  a6.access() = Mmc::Arg_cmd6_switch::Write_byte;
  a6.index() = idx;
  a6.value() = val;
  a6.cmdset() = 0;
  cmd->init_arg(Mmc::Cmd6_switch, a6.raw);
  if (with_status)
    cmd->flags.status_after_switch() = 1;
  cmd_exec(cmd);
  if (cmd->status == Cmd::Success)
    {
      for (unsigned i = 0; i < 50; ++i)
        {
          cmd->init_arg(Mmc::Cmd13_send_status, _rca << 16);
          cmd_exec(cmd);
          if (cmd->error())
            {
              // Unexpected error.
              warn.printf("\033[31mSWITCH/%d error '%s'.\n",
                          (unsigned)idx, cmd->str_status().c_str());
              return;
            }
          if (cmd->switch_error())
            {
              // Just report this error to the caller. It might be expected.
              warn.printf("\033[31mSWITCH/%d %s.\033[m\n",
                          (unsigned)idx, cmd->str_status().c_str());
              return;
            }
          if (cmd->mmc_status().ready_for_data())
            break;

          ++_prg_cnt;
          ++_prg_map[idx];
          _drv.delay(1);
        }
    }
}

template <class Driver>
void
Device<Driver>::mmc_app_cmd(Cmd *cmd, l4_uint32_t cmdval, l4_uint32_t arg,
                            l4_uint32_t datalen, l4_uint64_t dataphys,
                            l4_addr_t datavirt)
{
  cmd->init_arg(Mmc::Cmd55_app_cmd, _rca << 16);
  cmd_exec(cmd);
  if (cmd->error())
    return; // caller will handle this

  if (datalen)
    cmd->init_data(cmdval, arg, datalen, dataphys, datavirt);
  else
    cmd->init_arg(cmdval, arg);
  cmd->mark_app_cmd();
  cmd_exec(cmd);
}

template <class Driver>
void
Device<Driver>::show_csd(Mmc::Reg_csd const &csd)
{
  l4_uint64_t size;
  l4_uint32_t bus_freq;
  l4_uint32_t read_bl_len;
  l4_uint32_t write_bl_len;
  switch (csd.csd_structure())
    {
    case 0:
      size = csd.s0.device_size();
      bus_freq = csd.s0.tran_speed();
      read_bl_len = 1U << csd.s0.read_bl_len();
      write_bl_len = 1U << csd.s0.write_bl_len();
      break;
    case 1:
      size = csd.s1.device_size();
      bus_freq = csd.s1.tran_speed();
      read_bl_len = 1U << csd.s1.read_bl_len();
      write_bl_len = 1U << csd.s1.write_bl_len();
      break;
    case 2:
    case 3:
      size = csd.s3.device_size();
      bus_freq = csd.s3.tran_speed();
      read_bl_len = 1U << csd.s3.read_bl_len();
      write_bl_len = 1U << csd.s3.write_bl_len();
      info.printf("eMMC spec version: %s\n",
                  csd.s3.spec_vers() >= 4 ? "4.0+" : "old");
      break;
    default:
      info.printf("Unknown CSD structure %d\n", csd.csd_structure());
      L4Re::throw_error(-L4_EINVAL, "Unknown CSD structure");
      break;
    }

  trace.printf("Max read block length: %u, max write block length: %u.\n",
               read_bl_len, write_bl_len);
  if (size)
    trace.printf("Device size (CSD): %s\n", Util::readable_size(size).c_str());
  info.printf("Bus clock frequency: %s\n", Util::readable_freq(bus_freq).c_str());
}

} // namespace Emmc
