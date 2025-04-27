/*
 * Copyright (C) 2014, 2016, 2018-2019, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/cxx/bitfield>

#include <l4/libblock-device/types.h>

namespace Ahci {

/** Feature register of a AHCI HBA
 */
struct Hba_features
{
  l4_uint32_t raw;
  CXX_BITFIELD_MEMBER_RO(31, 31, s64a, raw); ///< 64-bit Addressing
  CXX_BITFIELD_MEMBER_RO(30, 30, sncq, raw); ///< Native Command Queueing
  CXX_BITFIELD_MEMBER_RO(29, 29, ssntf, raw);///< Supports SNotification Register
  CXX_BITFIELD_MEMBER_RO(28, 28, smps, raw); ///< Supports Mechanical Presence Switch
  CXX_BITFIELD_MEMBER_RO(27, 27, sss, raw);  ///< Supports Staggered Spin-up
  CXX_BITFIELD_MEMBER_RO(26, 26, salp, raw); ///< Supports Aggressive Link Power Management
  CXX_BITFIELD_MEMBER_RO(25, 25, sal, raw);  ///< Supports Activity LED
  CXX_BITFIELD_MEMBER_RO(24, 24, sclo, raw); ///< Supports Command List Override
  CXX_BITFIELD_MEMBER_RO(20, 23, iss, raw);  ///< Interface Speed Support
  CXX_BITFIELD_MEMBER_RO(18, 18, sam, raw);  ///< Supports AHCI mode only
  CXX_BITFIELD_MEMBER_RO(17, 17, spm, raw);  ///< Supports Port Multiplier
  CXX_BITFIELD_MEMBER_RO(16, 16, fbss, raw); ///< FIS-based Switching Supported
  CXX_BITFIELD_MEMBER_RO(15, 15, pmd, raw);  ///< PIO Multiple DRQ Block
  CXX_BITFIELD_MEMBER_RO(14, 14, ssc, raw);  ///< Slumber State Capable
  CXX_BITFIELD_MEMBER_RO(13, 13, psc, raw);  ///< Partial State Capable
  CXX_BITFIELD_MEMBER_RO( 8, 12, ncs, raw);  ///< Number of Command Slots
  CXX_BITFIELD_MEMBER_RO( 8,  8, cccs, raw); ///< Command Completion Coalescing Supported
  CXX_BITFIELD_MEMBER_RO( 6,  6, ems, raw);  ///< Enclosure Management Supported
  CXX_BITFIELD_MEMBER_RO( 5,  5, sxs, raw);  ///< Supports External SATA
  CXX_BITFIELD_MEMBER_RO( 0,  4, np, raw);   ///< Number of Ports

  explicit Hba_features(l4_uint32_t v) : raw(v) {}
};

namespace Regs {

namespace Hba {

/** Generic host control registers */
enum Regs
{
  Cap       = 0x00, ///< HBA capabilities
  Ghc       = 0x04, ///< global HBA control
  Is        = 0x08, ///< interrupt status register
  Pi        = 0x0c, ///< ports implemented
  Vs        = 0x10, ///< AHCI version
  Ccc_ctl   = 0x14, ///< Command Completion Coalescing Control
  Ccc_ports = 0x18, ///< Command Completion Coalescing Ports
  Em_loc    = 0x1c, ///< Enclosure Management Location
  Em_ctl    = 0x20, ///< Enclosure Management Control
  Cap2      = 0x24, ///< extended HBA capabilities
  Bohc      = 0x28, ///< BIOS/OS handoff and status
};

enum Ghc_reg
{
  Ghc_ae   = (1 << 31),  ///< AHCI enable
  Ghc_mrsm = (1 << 2),   ///< MSI Revert to Single Message
  Ghc_ie   = (1 << 1),   ///< Interrupt Enable
  Ghc_hr   = (1 << 0)    ///< HBA Reset
};

} // namespace Hba

namespace Port {

enum Regs
{
  Clb       = 0x00, ///< Command List Base Address
  Clbu      = 0x04, ///< Command List Base Address Upper 32-Bits
  Fb        = 0x08, ///< FIS Base Address
  Fbu       = 0x0C, ///< FIS Base Address Upper 32-bits
  Is        = 0x10, ///< Interrupt Status
  Ie        = 0x14, ///< Interrupt Enable
  Cmd       = 0x18, ///< Command and Status
  Tfd       = 0x20, ///< Task File Data
  Sig       = 0x24, ///< Signature
  Ssts      = 0x28, ///< Serial ATA Status
  Sctl      = 0x2c, ///< Serial ATA Control
  Serr      = 0x30, ///< Serial ATA Error
  Sact      = 0x34, ///< Serial ATA Active
  Ci        = 0x38, ///< Command Issue
  Sntf      = 0x3C, ///< Serial ATA Notification
  Fbs       = 0x40, ///< FIS-based Switching Control
  Devslp    = 0x44, ///< Device Sleep
  Vs        = 0x70, ///< Vendor Specific
};

enum Cmd_reg
{
  Cmd_icc   = (1 << 28), ///< Interface Communication Control
  Cmd_asp   = (1 << 27), ///< Aggressive Slumber / Partial
  Cmd_alpe  = (1 << 26), ///< Aggressive Link Power Management Enable
  Cmd_dlae  = (1 << 25), ///< Drive LED on ATAPI Enable
  Cmd_atapi = (1 << 24), ///< Device is ATAPI
  Cmd_apste = (1 << 23), ///< Automatic Partial to Slumber Transitions Enabled
  Cmd_fbscp = (1 << 22), ///< FIS-based Switching Capable Port
  Cmd_esp   = (1 << 21), ///< External SATA Port
  Cmd_cpd   = (1 << 20), ///< Cold Presence Detection
  Cmd_mpsp  = (1 << 19), ///< Mechanical Presence Switch Attached to Port
  Cmd_hpcp  = (1 << 18), ///< Hot Plug Capable Port
  Cmd_pma   = (1 << 17), ///< Port Multiplier Attached
  Cmd_cps   = (1 << 16), ///< Cold Presence State
  Cmd_cr    = (1 << 15), ///< Command List Running
  Cmd_fr    = (1 << 14), ///< FIS Receive Running
  Cmd_mpss  = (1 << 13), ///< Mechanical Presence Switch State
  Cmd_ccs   = (1 << 8),  ///< Current command Slot
  Cmd_fre   = (1 << 4),  ///< FIS Receive Enable
  Cmd_clo   = (1 << 3),  ///< Command List Override
  Cmd_pod   = (1 << 2),  ///< Power On Device
  Cmd_sud   = (1 << 1),  ///< Spin-Up Device
  Cmd_st    = (1 << 0),  ///< Start
};

enum Tfd_reg
{
    Tfd_sts_err = (1 << 0),  ///< Transfer error
    Tfd_sts_drq = (1 << 3),  ///< Data transfer requested
    Tfd_sts_bsy = (1 << 7),  ///< Interface is busy
};

enum Port_interrupts
{
    Is_cpds  = (1 << 31), ///< Cold Port Detect Status
    Is_tfes  = (1 << 30), ///< Task File Error Status
    Is_hbfs  = (1 << 29), ///< Host Bus Fatal Error Status
    Is_hbds  = (1 << 28), ///< Host Bus Data Error Status
    Is_ifs   = (1 << 27), ///< Interface Fatal Error Status
    Is_infs  = (1 << 26), ///< Interface Non-fatal Error Status
    Is_ofs   = (1 << 24), ///< Overflow Status
    Is_ipms  = (1 << 23), ///< Incorrect Port Multiplier Status
    Is_prcs  = (1 << 22), ///< PhyRdy Change Status
    Is_dmps  = (1 << 7),  ///< Device Mechanical Presence Status
    Is_pcs   = (1 << 6),  ///< Port Connect Change Status
    Is_dps   = (1 << 5),  ///< Descriptor Processed
    Is_ufs   = (1 << 4),  ///< Unknown FIS Interrupt
    Is_sdbs  = (1 << 3),  ///< Set Device Bits Interrupt
    Is_dss   = (1 << 2),  ///< DMA Setup FIS Interrupt
    Is_pss   = (1 << 1),  ///< PIO Setup FIS Interrupt
    Is_dhrs  = (1 << 0),  ///< Device to Host Register FIS Interrupt

    Is_mask_status = Is_cpds | Is_prcs | Is_dmps | Is_pcs,
    Is_mask_fatal  = Is_tfes | Is_hbfs | Is_hbds | Is_ifs,
    Is_mask_error  = Is_infs | Is_ofs,
    Is_mask_data   = Is_dps | Is_ufs | Is_sdbs | Is_dss
                      | Is_pss | Is_dhrs,
    Is_mask_nonfatal = Is_mask_status | Is_mask_error | Is_mask_data
};


} // namespace Port

} // namespace Regs

namespace Fis {

using Callback = Block_device::Inout_callback;
using Datablock = Block_device::Inout_block;

enum Command_header_flags
{
  Chf_prefetchable  = 0x1,
  Chf_write         = 0x2,
  Chf_atapi         = 0x4,
  Chf_reset         = (1 << 3),
  Chf_clr_busy      = (1 << 4),
};

/**
 * IO task
 */
struct Taskfile
{
  // command info
  l4_uint64_t lba; // 48bits, actually
  l4_uint16_t features;
  l4_uint16_t count;
  l4_uint8_t device;
  l4_uint8_t command;
  l4_uint8_t icc; // time limit
  l4_uint8_t control;

  unsigned flags;

  // data
  Block_device::Inout_block const *data;
  l4_size_t sector_size;
};

} // namespace Fis

} // namespace Ahci
