/*
 * Copyright (C) 2014, 2017-2021, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/cxx/bitfield>
#include <l4/cxx/utils>
#include <l4/drivers/hw_mmio_register_block>
#include <l4/util/atomic.h>
#include <l4/re/dma_space>
#include <l4/re/rm>
#include <l4/re/util/shared_cap>
#include <l4/re/util/unique_cap>
#include <l4/sys/cache.h>
#include <cassert>
#include <vector>

#include "ahci_types.h"
#include "debug.h"

#include <l4/libblock-device/errand.h>

namespace Ahci {

class Hba;

/**
 * Entry in the command list structure sent to the AHCI HBA.
 */
struct Command_header
{
  l4_uint32_t flags;
  /** Length of physical region descriptor table */
  CXX_BITFIELD_MEMBER(16, 31, prdtl, flags);
  /** Port multiplier port */
  CXX_BITFIELD_MEMBER(12, 15, pmp, flags);
  /** Clear busy upon ok */
  CXX_BITFIELD_MEMBER(10, 10, c, flags);
  /** Command is BIST FIS */
  CXX_BITFIELD_MEMBER( 9,  9, b, flags);
  /** Reset */
  CXX_BITFIELD_MEMBER( 8,  8, r, flags);
  /** Prefetchable */
  CXX_BITFIELD_MEMBER( 7,  7, p, flags);
  /** Direction is device write */
  CXX_BITFIELD_MEMBER( 6,  6, w, flags);
  /** Command is an ATAPI command */
  CXX_BITFIELD_MEMBER( 5,  5, a, flags);
  /** Command FIS length in double words */
  CXX_BITFIELD_MEMBER( 0,  4, cfl, flags);
  /** Physical region descriptor byte count */
  l4_uint32_t prdbc;
  /** Command table base address - lower 32 bit */
  l4_uint32_t ctba0;
  /** Command table base address - upper 32 bit */
  l4_uint32_t ctba0_u0;
  /** Reserved */
  l4_uint32_t reserved[4];
};

static_assert(sizeof(struct Command_header) == 32,
              "Command_header structure wrongly packed.");


/**
 * Command table for a single request to the AHCI HBA.
 */
struct Command_table
{
  enum
  {
    /** Maximum number of blocks in the command table */
    Max_entries = 24,
  };

  /** Command FIS structure */
  l4_uint8_t cfis[64];
  /** ATAPI command structure */
  l4_uint8_t acmd[64]; // only up to 16 bytes actually used
  struct
  {
    /** data base address - lower 32 bit */
    l4_uint32_t dba;
    /** data base address - upper 32 bit */
    l4_uint32_t dbau;
    l4_uint32_t reserved;
    /** byte count of block (size -1) */
    l4_uint32_t dbc;
  } prd[Max_entries];
};

static_assert(0x200 == sizeof(struct Command_table),
              "Command table wrongly packed.");



//--------------------------------------------
//  Command slot
//--------------------------------------------

/**
 * The command description that will be transmitted to the HBA.
 *
 * \note Currently this is implemented with a 1:1 relationship between
 *       command header and command table, i.e. the command table that is
 *       used by each header is fixed. That may not be the best implementation
 *       because it also restricts the number of scatter-gather entries to
 *       a fixed size.
 */
class Command_slot
{
public:
  /**
   * Set up a new command slot at the given memory regions.
   *
   * \param cmd_header    Pointer to where the command header structure
   *                      resides, if 0 then the slot is considered inactive.
   * \param cmd_table     Pointer to the command table to use
   * \param cmd_table_pa  Physical address of the command table.
   */
  Command_slot(Command_header *cmd_header, Command_table *cmd_table,
               l4_addr_t cmd_table_pa)
  : _cmd_table(cmd_table),
    _cmd_table_pa(cmd_table_pa),
    _cmd_header(cmd_header),
    _callback(0),
    _is_busy(1)
  {}

  /**
   * Mark command slot as free.
   */
  void release()
  {
    _callback = 0;
    cxx::write_now(&_is_busy, 0U);
  }

  /**
   * Return true if the command slot is in use.
   */
  bool is_busy() const { return cxx::access_once(&_is_busy); }

  /**
   * Try to reserve the command slot.
   *
   * \retval true   The slot could be reserved.
   * \retval false  The slot could not be reserved.
   */
  bool reserve()
  {
    if (!_cmd_header)
      return false;

    return l4util_cmpxchg32(&_is_busy, 0, 1);
  }

  /**
   * Fill command header and table from a taskfile.
   *
   * \param task    The command description.
   * \param cb      Object to inform when the task is finished.
   * \param port    Port number to use for port-multipliers.
   *
   * \pre the taskfile is assumed to be correct, no sanity check of parameters
   *          will be done with the exception of the number of blocks.
   */
  int setup_command(Fis::Taskfile const &task, Fis::Callback const &cb,
                    l4_uint8_t port);

  /**
   * Fill data table from a FIS datablock structure.
   *
   * \param data         Chained list of data block descriptors.
   * \param sector_size  Size of a logical sector in bytes.
   */
  int setup_data(Fis::Datablock const &data, l4_uint32_t sector_size);

  /**
   * Called when the task in this slot has been finished.
   */
  void command_finish()
  {
    // Deferred execution because we might be in the interrupt handler.
    if (_callback)
      Block_device::Errand::schedule(std::bind(_callback, L4_EOK, _cmd_header->prdbc), 0);

    release();
  }

  /**
   * Abort an on-going data transfer.
   *
   * Null operation if no data transfer was pending.
   */
  void abort()
  {
    if (is_busy())
      {
        l4_size_t out = _cmd_header->prdbc;

        // XXX check if the transfer is maybe done already?
        if (_callback)
          _callback(-L4_EIO, out);

        release();
      }
  }

private:
  Command_table *_cmd_table;
  l4_addr_t _cmd_table_pa;
  Command_header *_cmd_header;
  Fis::Callback _callback;
  l4_uint32_t _is_busy;
};


/**
 * A single port on a AHCI hba.
 */
class Ahci_port
{
  struct Command_data
  {
    Command_header headers[32];
    unsigned char  fis[256]; // FIS receive area
    Command_table  tables[];

    void dma_flush(unsigned slot)
    {
      l4_cache_dma_coherent(reinterpret_cast<unsigned long>(&headers[slot]),
                            reinterpret_cast<unsigned long>(&headers[slot + 1]));
      l4_cache_dma_coherent(reinterpret_cast<unsigned long>(&tables[slot]),
                            reinterpret_cast<unsigned long>(&tables[slot + 1]));


    }
  };

public:
  typedef L4drivers::Register_block<32> Port_regs;

  enum Device_type
  {
    Ahcidev_none    = 0,
    Ahcidev_ata     = 1,
    Ahcidev_atapi   = 2,
    Ahcidev_pmp     = 3,
    Ahcidev_semb    = 4,
    Ahcidev_unknown = 5,
  };

  enum State
  {
    S_undefined,    ///< no hardware association
    S_present,      ///< IO address has been assigned
    S_present_init, ///< Initializing during device discovery
    S_attached,     ///< Device discovery finished
    S_disabled,     ///< Port ready but device still disabled
    S_enabling,     ///< Device is being enabled
    S_disabling,    ///< Device is being disabled
    S_ready,        ///< Ready for IO commands
    S_error,        ///< IO error occurred, reset required
    S_error_init,   ///< Reinitilizing after failure
    S_fatal,        ///< Fatal IO error occurred, not-recoverable
  };

  /// Create a new unattached port.
  Ahci_port() : _devtype(Ahcidev_none), _state(S_undefined) {}

  /**
   * Attach the port to a HBA.
   *
   * \param base_addr     (Virtual) base address of the port registers.
   * \param buswidth      Width of address bus.
   * \param dma_space     Dma space to use for this device.
   */
  int attach(l4_addr_t base_addr, unsigned buswidth,
             L4Re::Util::Shared_cap<L4Re::Dma_space> const &dma_space);

  /**
   * Set up the data structures for the AHCI data transfer.
   *
   * \param maxslots The maximum number of slots the HBA allows to use.
   *
   * \throws L4::Runtime_error Resource allocation failed
   *                           or device unavailable.
   */
  void initialize_memory(unsigned maxslots);

  /**
   * Start a reinitialization of the port.
   *
   * \param callback Errand to schedule when reset is finished.
   *
   * This is the softest variant of a reset, that just tries to disable the
   * start register of the device.
   */
  void initialize(Block_device::Errand::Callback const &callback);

  /** Check that the device is ready for receiving commands. */
  bool is_ready() const { return _state == S_ready; }

  /**
   * Start a soft port reset.
   *
   * \param callback Errand to execute after the port reset has been finished.
   *
   * Aborts any ongoing operation and attempts a full port reset.
   *
   * This function only schedules a new errand and returns. It is the
   * responsibility of the callback to check if the device is in a well-defined
   * state after the reset (using is_port_idle()). If this is not the
   * case, then the HBA needs to be reset in order to return the port
   * into a working state.
   */
  void reset(Block_device::Errand::Callback const &callback);

  /**
   * Return true if device is present and communication established.
   */
  bool device_present() const { return device_state() == 3; }

  /**
   * Check that a device is attached and ready.
   */
  bool device_ready() const
  {
    return (_devtype != Ahcidev_none) && (device_state() == 3);
  }

  /**
   * Return what kind of device is attached to the port.
   */
  Device_type device_type() const { return _devtype; }

  /**
   * Return the width of the bus supported by the device
   *
   * \return Width in bits, one of 32 or 64.
   */
  unsigned char bus_width() const { return _buswidth; }

  /**
   * Place a new command.
   *
   * \param task     Task to execute.
   * \param cb       Callback to execute when the task is finished.
   * \param port     For port multipliers: destination port.
   *
   * \retval >=0  The slot number used for the task.
   * \retval <0   Error code.
   *
   * Finds a free slot and starts placing the command. If the
   * synchronous bit has been set, the function waits for the transfer
   * to complete, otherwise it returns immediately and calls the
   * optional callback given in cb on completion.
   */
  int send_command(Fis::Taskfile const &task, Fis::Callback const &cb,
                   l4_uint8_t port = 0);


  /**
   * Process all pending interrupts for this port.
   */
  int process_interrupts();

  /**
   * Start to put port into processing mode.
   *
   * \param callback Errand to schedule after the initialization has finished.
   *
   * \return L4_EOK if the device was already enabled, the callback will not be
   *         called in this case. -L4_EBUSY if the enable process has been
   *         started successfully, another negative error code otherwise.
   */
  void enable(Block_device::Errand::Callback const &callback);

  L4::Cap<L4Re::Dma_space> dma_space() const { return _dma_space.get(); }

  int dma_map(L4::Cap<L4Re::Dataspace> ds, l4_addr_t offset, l4_size_t size,
              L4Re::Dma_space::Direction dir,
              L4Re::Dma_space::Dma_addr *phys);

  int dma_unmap(L4Re::Dma_space::Dma_addr phys, l4_size_t size,
                L4Re::Dma_space::Direction dir);

  unsigned max_slots() const
  { return _slots.size(); }

private:
  /** Check if the HBA is processing IO tasks. */
  bool is_started() const
  {
    return _regs[Regs::Port::Cmd] & Regs::Port::Cmd_st;
  }

  /** Return true if the AHCI port has no requests pending. */
  bool is_port_idle() const
  {
    return !(_regs[Regs::Port::Tfd] & Regs::Port::Tfd_sts_bsy);
  }

  /** Return true if no command list override is in progress. */
  bool no_command_list_override() const
  {
    return !(_regs[Regs::Port::Cmd] & Regs::Port::Cmd_clo);
  }

  /** Return true if the command list is disabled. */
  bool is_command_list_disabled() const
  {
    return !(_regs[Regs::Port::Cmd]
             & (Regs::Port::Cmd_cr | Regs::Port::Cmd_st));
  }

  /** Return true if receiving a FIS is disabled. */
  bool is_fis_receive_disabled() const
  {
    return !(_regs[Regs::Port::Cmd]
             & (Regs::Port::Cmd_fr | Regs::Port::Cmd_fre));
  }

  /** Return the command slot currently being processed. */
  unsigned current_command_slot() const
  {
    return (_regs[Regs::Port::Cmd] >> 8) & 0x1F;
  }

  /** Return the state of the device as reported by the hardware. */
  unsigned device_state() const { return _regs[Regs::Port::Ssts] & 0xF; }

  /**
   * Checks all slots for commands that have been finished.
   */
  void check_pending_commands()
  {
    l4_uint32_t slotstate = _regs[Regs::Port::Ci];

    for (auto &s : _slots)
      {
        if (s.is_busy() && ((slotstate & 1) == 0))
          s.command_finish();
        slotstate >>= 1;
      }
  }

  void handle_error();

  void disable_fis_receive(Block_device::Errand::Callback const &callback);

  void wait_tfd(Block_device::Errand::Callback const &callback);

  void dma_enable(Block_device::Errand::Callback const &callback);

  /**
   * Enable all interrupts on this port.
   */
  void enable_ints()
  {
    if (_devtype != Ahcidev_none)
      _regs[Regs::Port::Ie] = Regs::Port::Is_mask_nonfatal;
  }

  /**
   * Put the port out of processing mode.
   *
   * \param callback Errand to schedule after the operation has finished.
   *
   * This function only stops the port, it does not notify
   * potentially pending clients.
   */
  void disable(Block_device::Errand::Callback const &callback);

  /**
   * Abort all pending operations and disable port.
   *
   * \param callback Errand to schedule after the abort has finished.
   */
  void abort(Block_device::Errand::Callback const &callback);

  /**
   * Dump port register set to debug (at trace level).
   */
  void dump_registers(L4Re::Util::Dbg const &log) const;

  Device_type _devtype;
  State _state;
  std::vector<Command_slot> _slots;
  Port_regs _regs;
  L4Re::Util::Unique_cap<L4Re::Dataspace> _cmddata_cap;
  L4Re::Rm::Unique_region<Command_data *> _cmd_data;
  L4Re::Dma_space::Dma_addr _cmddata_paddr;
  L4Re::Util::Shared_cap<L4Re::Dma_space> _dma_space;
  unsigned char _buswidth;
};

}
