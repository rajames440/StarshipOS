/*
 * Copyright (C) 2014-2015, 2017-2021, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/re/env>
#include <l4/re/error_helper>

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci.h>
#include <cstring>
#include <endian.h>

#include "ahci_port.h"
#include "debug.h"

#if (__BYTE_ORDER == __BIG_ENDIAN)
# error "Big endian byte order not implemented."
#endif

static Dbg trace(Dbg::Trace, "ahci-port");

namespace Errand = Block_device::Errand;

namespace Ahci {

//--------------------------------------------
//  Command slot
//--------------------------------------------

int
Command_slot::setup_command(Fis::Taskfile const &task, Fis::Callback const &cb,
                            l4_uint8_t port)
{
  // fill command table
  l4_uint8_t *fis = _cmd_table->cfis;
  fis[0] = 0x27;                    // FIS type Host-to-Device
  fis[1] = (1 << 7) | (port & 0xF); // upper bit defines command FIS
  fis[2] = task.command;
  fis[3] = task.features & 0xFF;
  fis[4] = task.lba & 0xFF;
  fis[5] = (task.lba >> 8) & 0xFF;
  fis[6] = (task.lba >> 16) & 0xFF;
  fis[7] = task.device;
  fis[8] = (task.lba >> 24) & 0xFF;
  fis[9] = (task.lba >> 32) & 0xFF;
  fis[10] = (task.lba >> 40) & 0xFF;
  fis[11] = (task.features >> 8) & 0xFF;
  fis[12] = task.count;
  fis[13] = (task.count >> 8) & 0xFF;
  fis[14] = task.icc;
  fis[15] = task.control;

  // now add the slot information
  _cmd_header->flags = 0;
  _cmd_header->prdtl() = 0;
  _cmd_header->p() = (task.flags & Fis::Chf_prefetchable) ? 1 : 0;
  _cmd_header->w() = (task.flags & Fis::Chf_write) ? 1 : 0;
  _cmd_header->a() = (task.flags & Fis::Chf_atapi) ? 1 : 0;
  _cmd_header->c() = (task.flags & Fis::Chf_clr_busy) ? 1 : 0;
  _cmd_header->cfl() = 5;
  _cmd_header->prdbc = 0;
  _cmd_header->ctba0 = _cmd_table_pa;
  if (sizeof(_cmd_table_pa) == 8)
    _cmd_header->ctba0_u0 = (l4_uint64_t) _cmd_table_pa >> 32;

  // save client info
  _callback = cb;

  return L4_EOK;
}

int
Command_slot::setup_data(Fis::Datablock const &data, l4_uint32_t sector_size)
{
#if (__BYTE_ORDER == __BIG_ENDIAN)
#error "Big endian not implemented."
#endif

  unsigned i = 0;
  for (Fis::Datablock const *block = &data;
       block && i < Command_table::Max_entries;
       ++i, block = block->next.get())
    {
      _cmd_table->prd[i].dba = block->dma_addr;
      if (sizeof(l4_addr_t) == 8)
        _cmd_table->prd[i].dbau = (l4_uint64_t) block->dma_addr >> 32;
      else
        _cmd_table->prd[i].dbau = 0;
      _cmd_table->prd[i].dbc = (block->num_sectors * sector_size) - 1;
      // TODO: cache: make sure client data is flushed
    }

  _cmd_header->prdtl() = i;

  return i;
}

//--------------------------------------------
//  Ahci_port
//--------------------------------------------


int
Ahci_port::attach(l4_addr_t base_addr, unsigned buswidth,
                  L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma_space)
{
  if (_state != S_undefined)
    return -L4_EEXIST;

  trace.printf("Attaching port to address 0x%lx\n", base_addr);

  _regs = new L4drivers::Mmio_register_block<32>(base_addr);
  _buswidth = buswidth;

  _state = S_present;

  if (!device_present())
    {
      trace.printf("Device not present @0x%lx. Device state 0x%x\n", base_addr,
                   device_state());
      _devtype = Ahcidev_none;
      return -L4_ENODEV;
    }

  // device type cannot be determined as long as the FIS buffer is not enabled!
  _devtype = Ahcidev_unknown;
  _dma_space = dma_space;

  return L4_EOK;
}


void
Ahci_port::initialize_memory(unsigned maxslots)
{
  if (_state != S_attached)
    L4Re::chksys(-L4_EIO, "Device encountered fatal error.");

  if (_devtype == Ahcidev_none)
    L4Re::chksys(-L4_ENODEV, "Device no longer available.");

  // disable all interrupts for now
  _regs[Regs::Port::Ie] = 0;

  // set up memory for the command data
  _cmddata_cap = L4Re::chkcap(L4Re::Util::make_unique_cap<L4Re::Dataspace>(),
                              "Allocate capability for command data.");
  auto *e = L4Re::Env::env();
  unsigned memsz = sizeof(Command_data) + maxslots * sizeof(Command_table);
  L4Re::chksys(e->mem_alloc()->alloc(memsz, _cmddata_cap.get(),
                                     L4Re::Mem_alloc::Continuous
                                     | L4Re::Mem_alloc::Pinned),
               "Allocate memory for command data.");

  L4Re::chksys(e->rm()->attach(&_cmd_data, memsz,
                               L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
                               L4::Ipc::make_cap_rw(_cmddata_cap.get()), 0,
                               L4_PAGESHIFT),
               "Attach command data memory.");

  L4Re::chksys(dma_map(_cmddata_cap.get(), 0, memsz,
                       L4Re::Dma_space::Direction::Bidirectional,
                       &_cmddata_paddr),
               "Attach command data memory to DMA space.");

  Dbg::trace().printf("Initializing port @%p.\n", _cmd_data.get());

  // setup command list
  l4_addr_t addr = _cmddata_paddr + offsetof(Command_data, headers);
  _regs[Regs::Port::Clb] = addr;
  _regs[Regs::Port::Clbu] = (sizeof(l4_addr_t) == 8)
                            ? ((l4_uint64_t) addr >> 32) : 0;

  // setup FIS receive region
  addr = _cmddata_paddr + offsetof(Command_data, fis);
  _regs[Regs::Port::Fb] = addr;
  _regs[Regs::Port::Fbu] = (sizeof(l4_addr_t) == 8)
                           ? ((l4_uint64_t) addr >> 32) : 0;

  // enable FIS buffer
  _regs[Regs::Port::Cmd].set(Regs::Port::Cmd_fre);

  // reset error register
  _regs[Regs::Port::Serr] = 0xFFFFFFFF;

  // Reading the device signature works only after the FIS buffer is enabled
  // by setting 'Cmd_fre' in PxCMD. On bare metal it might be even required to
  // trigger a D2H register FIS transfer (needs testing!) but not on QEMU.
  l4_uint32_t tmp = _regs[Regs::Port::Sig];
  unsigned lbah = (tmp >> 24) & 0xff;
  unsigned lbam = (tmp >> 16) & 0xff;

  // detect device type (borrowed from Linux)
  if ((lbam == 0) && (lbah == 0))
    _devtype = Ahcidev_ata;
  else if ((lbam == 0x14) && (lbah == 0xeb))
    _devtype = Ahcidev_atapi;
  else if ((lbam == 0x69) && (lbah == 0x96))
    _devtype = Ahcidev_pmp;
  else if ((lbam == 0x3c) && (lbah == 0xc3))
    _devtype = Ahcidev_semb;
  else
    _devtype = Ahcidev_unknown;

  // Initialize command slots
  _slots.clear();
  _slots.reserve(maxslots);
  // to be available CI and SACT must be cleared
  l4_uint32_t state = _regs[Regs::Port::Ci] | _regs[Regs::Port::Sact];

  // physical address, used for pointer arithmetic
  l4_addr_t phys_ct = _cmddata_paddr + offsetof(Command_data, tables);
  Command_data *cd = _cmd_data.get();
  for (unsigned i = 0; i < maxslots; ++i)
    {
      _slots.emplace_back(&cd->headers[i], &cd->tables[i],
                          phys_ct + i * sizeof(Command_table));

      if (!(state & (1 << i)))
        _slots[i].release();
    }

  _state = S_disabled;

  Dbg::trace().printf("Initialization finished.\n");
  dump_registers(trace);
}



void
Ahci_port::enable(Errand::Callback const &callback)
{
  if (_state != S_disabled)
    {
      // TODO should if be fatal if this is called in unexpected states?
      callback();
      return;
    }

  _state = S_enabling;

  if (L4_UNLIKELY(!is_port_idle()))
    {
      _regs[Regs::Port::Cmd].set(Regs::Port::Cmd_clo);
      Errand::poll(10, 50000,
                   std::bind(&Ahci_port::no_command_list_override, this),
                   [=](bool ret)
                     {
                       if (_state != S_enabling)
                         {
                           Dbg::warn().printf("Unexpected state in Ahci_port::enable\n");
                           callback();
                         }
                       else if (ret)
                         dma_enable(callback);
                       else
                         {
                           _state = S_fatal;
                           callback();
                         }
                     });
    }
  else
    dma_enable(callback);
}


void
Ahci_port::dma_enable(Errand::Callback const &callback)
{
  _regs[Regs::Port::Cmd].set(Regs::Port::Cmd_st);

  if (_state == S_enabling)
    {
      enable_ints();
      _state = S_ready;
    }
  else
    {
      Dbg::warn().printf("Unexpected state in Ahci_port::enable\n");
    }

  callback();
}


void
Ahci_port::disable(Errand::Callback const &callback)
{
  if (_state == S_disabled || _state == S_error)
    {
      _state = S_fatal;
      Err().printf("Port disable called in unexpected state.\n");
    }

  if (is_command_list_disabled())
    {
      _state = S_disabled;
      callback(); // already disabled
      return;
    }

  // disable interrupts
  _regs[Regs::Port::Ie] = 0;
  // disable DMA engine
  _regs[Regs::Port::Cmd].clear(Regs::Port::Cmd_st);

  if (is_command_list_disabled())
    {
      _state = S_disabled;
      callback();
      return;
    }

  _state = S_disabling;

  Errand::poll(10, 50000,
               std::bind(&Ahci_port::is_command_list_disabled, this),
               [=](bool ret)
                 {
                   if (_state != S_disabling)
                     Dbg::warn().printf("Unexpected state in Ahci_port::disable");
                   else if (ret)
                     _state = S_disabled;
                   else
                     {
                       _state = S_fatal;
                       Err().printf("Could not disable port.");
                     }
                   callback();
                 });
}


void
Ahci_port::abort(Errand::Callback const &callback)
{
  // disable the port and then cancel any outstanding requests
  disable(
    [=]()
      {
        trace.printf("START ERRAND Abort_slots_errand\n");
        for (auto &s : _slots)
          s.abort();

        callback();
      });
}

void
Ahci_port::dump_registers(L4Re::Util::Dbg const &log) const
{
  log.printf(" CLB: 0x%08x - 0x%08x\n",
             _regs[Regs::Port::Clbu].read(), _regs[Regs::Port::Clb].read());
  log.printf("  FB: 0x%08x - 0x%08x\n",
             _regs[Regs::Port::Fbu].read(), _regs[Regs::Port::Fb].read());
  log.printf("  IS: 0x%08x    IE: 0x%08x\n",
             _regs[Regs::Port::Is].read(), _regs[Regs::Port::Ie].read());
  log.printf(" CMD: 0x%08x   TFD: 0x%08x\n",
             _regs[Regs::Port::Cmd].read(), _regs[Regs::Port::Tfd].read());
  log.printf(" SIG: 0x%08x    VS: 0x%08x\n",
             _regs[Regs::Port::Sig].read(), _regs[Regs::Port::Vs].read());
  log.printf("SSTS: 0x%08x  SCTL: 0x%08x\n",
             _regs[Regs::Port::Ssts].read(), _regs[Regs::Port::Sctl].read());
  log.printf("SERR: 0x%08x  SACT: 0x%08x\n",
             _regs[Regs::Port::Serr].read(), _regs[Regs::Port::Sact].read());
  log.printf("  CI: 0x%08x  SNTF: 0x%08x\n",
             _regs[Regs::Port::Ci].read(), _regs[Regs::Port::Sntf].read());
  log.printf(" FBS: 0x%08x  SLEP: 0x%08x\n",
             _regs[Regs::Port::Fbs].read(), _regs[Regs::Port::Devslp].read());
}


void
Ahci_port::initialize(Errand::Callback const &callback)
{
  if (_state == S_present)
    _state = S_present_init;
  else if (_state == S_error)
    _state = S_error_init;
  else
    {
      Err().printf("'Initialize' called out of order.\n");
      _state = S_fatal;
      return;
    }

  trace.printf("Port: starting reset\n");
  if (is_command_list_disabled())
    {
      disable_fis_receive(callback);
      return;
    }

  _regs[Regs::Port::Cmd].clear(Regs::Port::Cmd_st);

  Errand::poll(10, 50000,
               std::bind(&Ahci_port::is_command_list_disabled, this),
               [=](bool ret)
                 {
                   if (!(_state == S_present_init || _state == S_error_init))
                     {
                       // TODO Should this unexpected state change be fatal?
                       Dbg::warn().printf("Unexpected state in Ahci_port::initialize\n");
                       callback();
                     }
                   else if (ret)
                     {
                       disable_fis_receive(callback);
                     }
                   else
                     {
                       Err().printf("Init: ST disable failed.\n");
                       dump_registers(trace);
                       _state = S_fatal;
                       callback();
                     }
                 });
}

void
Ahci_port::disable_fis_receive(Errand::Callback const &callback)
{
  if (is_fis_receive_disabled())
    {
      _state = (_state == S_present_init) ? S_attached : S_disabled;
      callback();
      return;
    }

  _regs[Regs::Port::Cmd].clear(Regs::Port::Cmd_fre);

  Errand::poll(10, 50000,
               std::bind(&Ahci_port::is_fis_receive_disabled, this),
               [=](bool ret)
                 {
                   if (!(_state == S_present_init || _state == S_error_init))
                     {
                       // TODO Should this unexpected state change be fatal?
                       Dbg::warn().printf("Unexpected state in "
                                          "Ahci_port::disable_fis_receive\n");
                     }
                   else if (ret)
                     _state = (_state == S_present_init) ?
                              S_attached : S_disabled;
                   else
                     {
                       Err().printf(" Reset: fis receive reset failed.\n");
                       _state = S_fatal;
                     }
                   callback();
                 }
              );
}


void
Ahci_port::reset(Errand::Callback const &callback)
{
  Dbg::info().printf("Doing full port reset.\n");

  _regs[Regs::Port::Sctl] = 1;

  // wait for 5ms, according to spec
  Errand::schedule([=]()
    {
      _regs[Regs::Port::Sctl] = 0;

      Errand::poll(10, 50000,
                   std::bind(&Ahci_port::device_present, this),
                   [=](bool ret)
                     {
                       if (ret)
                         wait_tfd(callback);
                       else
                         callback();
                     });
    }, 5);

}

void
Ahci_port::wait_tfd(Errand::Callback const &callback)
{
  Errand::poll(10, 50000,
               std::bind(&Ahci_port::is_port_idle, this),
               [=](bool ret)
                 {
                   if (ret)
                     {
                       _regs[Regs::Port::Serr] = 0xFFFFFFFF;
                       _regs[Regs::Port::Is] = 0xFFFFFFFF;
                     }
                   callback();
                 });

}


int
Ahci_port::send_command(Fis::Taskfile const &task, Fis::Callback const &cb,
                        l4_uint8_t port)
{
  if (L4_UNLIKELY(!device_ready()))
    return -L4_ENODEV;

  unsigned slot = 0;
  for (auto &s : _slots)
    {
      if (s.reserve())
        {
          s.setup_command(task, cb, port);
          if (s.setup_data(*task.data, task.sector_size) < 0)
            {
              Err().printf("Bad data blocks\n");
              s.release();
              return -L4_EINVAL;
            }
          trace.printf("Reserved slot %d.\n", slot);
          if (is_ready())
            {
              trace.printf("Sending off slot %d.\n", slot);
              _cmd_data.get()->dma_flush(slot);
              _regs[Regs::Port::Ci] = 1 << slot;
            }
          else
            {
              // TODO If the mode is enabling, should we wait?
              trace.printf("Device not ready for serving slot %d.\n", slot);
              _slots[slot].abort();
            }

          return slot;
        }
      ++slot;
    }

  return -L4_EBUSY;
}


int
Ahci_port::process_interrupts()
{
  if (_devtype == Ahcidev_none)
    {
      Dbg::warn().printf("Interrupt for inactive port received.\n");
      return -L4_ENODEV;
    }

  l4_uint32_t istate = _regs[Regs::Port::Is];

  if (istate & Regs::Port::Is_mask_status)
    {
      Dbg::warn().printf("Device state changed.\n");
      // state changed: clear interrupts
      _regs[Regs::Port::Is] = istate & Regs::Port::Is_mask_status;
      // TODO Restart the device detection cycle here.
      abort([=]{ reset([]{}); });
      // XXX this should be propagated to the driver running the device
      return -L4_EIO;
    }

  if (istate & (Regs::Port::Is_mask_fatal | Regs::Port::Is_mask_error))
    {
      // error: clear interrupts
      _regs[Regs::Port::Is]
        = istate & (Regs::Port::Is_mask_fatal | Regs::Port::Is_mask_error);
      handle_error();
    }
  else
    {
      // data: clear interrupts
      _regs[Regs::Port::Is] = Regs::Port::Is_mask_data;
      check_pending_commands();
    }

  return L4_EOK;
}

void
Ahci_port::handle_error()
{
  // find the commands that are still pending
  l4_uint32_t slotstate = _regs[Regs::Port::Ci];

  if (is_started())
    {
      // If the port is still active, abort the failing task
      // and try to safe the rest.
      _slots[current_command_slot()].abort();

      check_pending_commands();
    }
  else
    {
      // Otherwise all tasks will be aborted.
      for (auto &s : _slots)
        s.abort();
      slotstate = 0;
    }

  _state = S_error;

  initialize(
    [=]()
      {
        // clear error register and error interrupts
        _regs[Regs::Port::Serr] = 0;
        _regs[Regs::Port::Is] = Regs::Port::Is_mask_fatal
                                | Regs::Port::Is_mask_error;
        enable(
          [=]()
            {
              // if all went well, reissue all commands that were
              // not aborted, otherwise abort everything
              if (slotstate)
                {
                  if (is_ready())
                    _regs[Regs::Port::Ci] = slotstate;
                  else
                    for (auto &s : _slots)
                      s.abort();
                }
            });
      });

}

int
Ahci_port::dma_map(L4::Cap<L4Re::Dataspace> ds, l4_addr_t offset,
                   l4_size_t size, L4Re::Dma_space::Direction dir,
                   L4Re::Dma_space::Dma_addr *phys)
{
  l4_size_t out_size = size;

  auto ret = _dma_space->map(L4::Ipc::make_cap_rw(ds), offset, &out_size,
                             L4Re::Dma_space::Attributes::None, dir, phys);

  if (ret < 0 || out_size < size)
    {
      *phys = 0;
      Dbg::info().printf("Cannot resolve physical address (ret = %ld, %zu < %zu).\n",
                         ret, out_size, size);
      return -L4_ENOMEM;
    }

  return L4_EOK;
}

int
Ahci_port::dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t size,
                     L4Re::Dma_space::Direction dir)
{
  return _dma_space->unmap(phys, size, L4Re::Dma_space::Attributes::None, dir);
}


}
