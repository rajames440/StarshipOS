/* SPDX-License-Identifier: GPL-2.0-only or License-Ref-kk-custom */
/*
 * Copyright (C) 2019, 2021-2022 Kernkonzept GmbH.
 * Author(s): Philipp Eppelt <philipp.epppelt@kernkonzept.com>
 */

/**
 * Atkins unit test for Cmdline::register_flag.
 *
 * No validation is performed, as these tests test the input validation of
 * Cmdline::register_flag.
 *
 * Note, as the flag registry is global and doesn't allow the same flag
 * character to be registered twice, all test flags must be unique in the whole
 * file.
 */

#include <l4/atkins/tap/main>
#include <l4/atkins/l4_assert>

namespace Tests { namespace Atkins_lib {

using namespace Atkins::Cmdline;

using pair_type = std::pair<char, Manager::Arg_state>;
static pair_type good_flags[] =
  {{'a', Manager::No_arg}, {'B', Manager::Req_arg}, {'c', Manager::Opt_arg}};

static pair_type bad_flags[] =
  {{':', Manager::Opt_arg},
   // reserved flags
   {'b', Manager::No_arg},
   {'v', Manager::No_arg},
   {'r', Manager::No_arg}};

static void callback(char const *) {};

/// Fixture for good flags test
class CmdlineFlags : public testing::TestWithParam<pair_type> {};

/// Format conformant, single letter and flags can be registered.
TEST_P(CmdlineFlags, SingleFlag)
{
  TAP_UUID("2c93c688-296d-4394-98d9-43b176a97e9d");

  char flag;
  Manager::Arg_state state;
  std::tie(flag, state) = GetParam();

  ASSERT_L4OK(cmdline()->register_flag(flag, state, callback))
    << "The single flag " << flag << " can be registered.";
}

static INSTANTIATE_TEST_SUITE_P(SingleValidFlag, CmdlineFlags,
                               testing::ValuesIn(good_flags));


/// Fixture for bad flags test
class BadCmdlineFlags : public testing::TestWithParam<pair_type> {};

/// Reserved and non-conformant flag values cannot be registered.
TEST_P(BadCmdlineFlags, SingleFlag)
{
  TAP_UUID("a20f8a3b-4e14-459d-8512-06bbde00cbe9");

  char flag;
  Manager::Arg_state state;
  std::tie(flag, state) = GetParam();

  ASSERT_L4ERR(L4_EINVAL, cmdline()->register_flag(flag, state, callback))
    << "The single flag " << flag << " is an invalid flag.";
}

static INSTANTIATE_TEST_SUITE_P(
  SingleInvalidFlag, BadCmdlineFlags, testing::ValuesIn(bad_flags));


/// A nullptr cannot be registered as callback function.
TEST(CmdlineFlags, InvalidCallback)
{
  TAP_UUID("114a235c-9e65-479b-b248-1a6cfaadeefe");

  ASSERT_L4ERR(L4_EINVAL, cmdline()->register_flag('Z', Manager::No_arg, nullptr))
    << "Cannot register a nullptr as callback";
}

}} // namespace Tests::Atkins
