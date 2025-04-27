/*
 * Copyright (C) 2021, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Simple control for Clock Pulse Generator on RCar3.
 */

#pragma once

#include <l4/drivers/hw_mmio_register_block>
#include <l4/vbus/vbus>

#include "iomem.h"

/**
 * Clock Pulse Generator.
 */
class Rcar3_cpg
{
public:
  Rcar3_cpg();
  int enable_clock(unsigned n, unsigned bit);
  void enable_register(unsigned reg, unsigned value);

  enum : unsigned
  {
    Sd2ckcr       = 0x268,      //< SD-IF2 clock frequency control register
  };

private:
  enum : unsigned
  {
    Cpgwpr        = 0x900,      //< CPG write protect register
    Cpgwpcr       = 0x904,      //< CPG write protect control register

    Nr_modules    =    12,      //< Number of module registers
                                //< (for Mstpsr, Rmstpcr, Smstpcr, Scmstpcr)

    Smstpcr0      = 0x130,      //< System module stop control register 0
    Smstpcr1      = 0x134,      //< System module stop control register 1
    Smstpcr2      = 0x138,      //< System module stop control register 2
    Smstpcr3      = 0x13c,      //< System module stop control register 3
    Smstpcr4      = 0x140,      //< System module stop control register 4
    Smstpcr5      = 0x144,      //< System module stop control register 5
    Smstpcr6      = 0x148,      //< System module stop control register 6
    Smstpcr7      = 0x14c,      //< System module stop control register 7
    Smstpcr8      = 0x990,      //< System module stop control register 8
    Smstpcr9      = 0x994,      //< System module stop control register 9
    Smstpcr10     = 0x998,      //< System module stop control register 10
    Smstpcr11     = 0x99c,      //< System module stop control register 11

    Mstpsr0       = 0x030,      //< Module stop status register 0
    Mstpsr1       = 0x038,      //< Module stop status register 1
    Mstpsr2       = 0x040,      //< Module stop status register 2
    Mstpsr3       = 0x048,      //< Module stop status register 3
    Mstpsr4       = 0x04c,      //< Module stop status register 4
    Mstpsr5       = 0x03c,      //< Module stop status register 5
    Mstpsr6       = 0x1c0,      //< Module stop status register 6
    Mstpsr7       = 0x1c4,      //< Module stop status register 7
    Mstpsr8       = 0x9a0,      //< Module stop status register 8
    Mstpsr9       = 0x9a4,      //< Module stop status register 9
    Mstpsr10      = 0x9a8,      //< Module stop status register 10
    Mstpsr11      = 0x9ac,      //< Module stop status register 11

    Rmstpcr0      = 0x110,      //< Realtime module stop control register 0
    Rmstpcr1      = 0x114,      //< Realtime module stop control register 1
    Rmstpcr2      = 0x118,      //< Realtime module stop control register 2
    Rmstpcr3      = 0x11c,      //< Realtime module stop control register 3
    Rmstpcr4      = 0x120,      //< Realtime module stop control register 4
    Rmstpcr5      = 0x124,      //< Realtime module stop control register 5
    Rmstpcr6      = 0x128,      //< Realtime module stop control register 6
    Rmstpcr7      = 0x12c,      //< Realtime module stop control register 7
    Rmstpcr8      = 0x980,      //< Realtime module stop control register 8
    Rmstpcr9      = 0x984,      //< Realtime module stop control register 9
    Rmstpcr10     = 0x988,      //< Realtime module stop control register 10
    Rmstpcr11     = 0x98c,      //< Realtime module stop control register 11
  };

  static constexpr unsigned rmstpcr[Nr_modules] =
  {
    Rmstpcr0, Rmstpcr1, Rmstpcr2, Rmstpcr3, Rmstpcr4, Rmstpcr5,
    Rmstpcr6, Rmstpcr7, Rmstpcr8, Rmstpcr9, Rmstpcr10, Rmstpcr11,
  };

  static constexpr unsigned smstpcr[Nr_modules] =
  {
    Smstpcr0, Smstpcr1, Smstpcr2, Smstpcr3, Smstpcr4, Smstpcr5,
    Smstpcr6, Smstpcr7, Smstpcr8, Smstpcr9, Smstpcr10, Smstpcr11
  };

  static constexpr unsigned mstpsr[Nr_modules] =
  {
    Mstpsr0, Mstpsr1, Mstpsr2, Mstpsr3, Mstpsr4, Mstpsr5,
    Mstpsr6, Mstpsr7, Mstpsr8, Mstpsr9, Mstpsr10, Mstpsr11,
  };

  L4drivers::Register_block<32> _regs;
};
