/* SPDX-License-Identifier: GPL-2.0-only or License-Ref-kk-custom */
/*
 * Copyright (C) 2019, 2022 Kernkonzept GmbH.
 * Author(s): Philipp Eppelt <philipp.epppelt@kernkonzept.com>
 */

/**
 * A test can register command line options with a callback for the inital
 * command line parsing done by atkins.
 *
 * \pre The ned script provides a cmdline argument "-e42.1234"
 */

#include <l4/atkins/l4_assert>
#include <l4/atkins/tap/main>
#include <l4/atkins/tap/cmdline>

#include <string>
#include <cmath>

double const Init_value = 31.0;
double const epsilon = 1e-13;

struct Globals
{
  static void set_value(char const *arg)
  {
    value = std::stod(arg);
  }

  static double value;
};
double Globals::value = Init_value;

/// The value is read from the command line and set via the callback.
TEST(CustomCmdlineArg, SetValue)
{
  TAP_UUID("5a5942b6-150d-4f44-93ab-f4ae06f5a6fa");

  ASSERT_NEAR(42.1234, Globals::value, epsilon)
    << "The command line parameter was read and the callback was executed.";
}

/// The value read from the command line is indeed not the initial value.
TEST(CustomCmdlineArg, InitValueOverwritten)
{
  TAP_UUID("0306a744-7744-4795-a601-6d46f5348a73");

  ASSERT_FALSE(std::fabs(Init_value - Globals::value) <= epsilon)
    << "The initial value is overwritten by the callback.";
}

namespace {
  /**
   * Register the expected command line parameters with Atkins' command line
   * manager.
   */
  struct Reg_cmdline
  {
    Reg_cmdline()
    {
      L4Re::chksys(Atkins::Cmdline::cmdline()
                     ->register_flag('e', Atkins::Cmdline::Manager::Req_arg,
                                     Globals::set_value),
                   "Cannot register 'e:' flag with cmdline\n");
    }
  };

  /// The ctor of static variables runs before main is invoked.
  static Reg_cmdline __reg_cmdline;
}
