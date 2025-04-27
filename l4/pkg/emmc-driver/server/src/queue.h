/*
 * Copyright (C) 2020, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

namespace Emmc {

// Command Queue Engine according to eMMC
namespace Cqe {

enum Regs
{
  Cqver         = 0x00, ///< CQ Version
  Cqcap         = 0x04, ///< CQ Capabilities
  Cqcfg         = 0x08, ///< CQ Configuration
  Cqctl         = 0x0c, ///< CQ Control
  Cqis          = 0x10, ///< CQ Interrupt Status
  Cqiste        = 0x14, ///< CQ Interrupt Status Enable
  Cqisge        = 0x18, ///< CQ Interrupt Signal Enable
  Cqic          = 0x1c, ///< CQ Interrupt Coalescing
  Cqtdlba       = 0x20, ///< CQ Task Descriptor List Base Address
  Cqtdlbau      = 0x24, ///< CQ Task Descriptor List Base Address Upper 32 bits
  Cqtdbr        = 0x28, ///< CQ Task Doorbell
  Cqtcn         = 0x2c, ///< CQ Task Completion Notification
  Cqdqs         = 0x30, ///< CQ Device Queue Status
  Cqdpt         = 0x34, ///< CQ Device Pending Tasks
  Cqtclr        = 0x38, ///< CQ Task Clear
  Cqssc1        = 0x40, ///< CQ Send Status Configuration 1
  Cqssc2        = 0x44, ///< CQ Send Status Configuration 2
  Cqcrdct       = 0x48, ///< CQ Command Response for Direct-Command Task
  Cqrmem        = 0x50, ///< CQ Response Mode Error Mask
  Cqterri       = 0x54, ///< CQ Task Error Information
  Cqcri         = 0x58, ///< CQ Command Response Index
  Cqcra         = 0x5c, ///< CQ Command Response Argument
};

} // namespace Cqe

} // namespace Emmc
