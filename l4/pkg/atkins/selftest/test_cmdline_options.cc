/* SPDX-License-Identifier: GPL-2.0-only or License-Ref-kk-custom */
/*
 * Copyright (C) 2019, 2022 Kernkonzept GmbH.
 * Author(s): Philipp Eppelt <philipp.epppelt@kernkonzept.com>
 */

/**
 * Atkins unit test for Cmdline::register_option.
 *
 * No validation is performed, as these tests test the input validation of
 * Cmdline::register_option.
 *
 * Note, as the option registry is global and doesn't allow the same option
 * name to be registered twice, all test option names and val value must be
 * unique in the whole file.
 */

#include <l4/atkins/tap/main>
#include <l4/atkins/l4_assert>

namespace Tests { namespace Atkins_lib {

static void callback(char const *) {};

using namespace Atkins::Cmdline;

/// New options can be registered with Atkins.
TEST(CmdlineOptionsGood, Options)
{
  TAP_UUID("4aafd294-1c00-4812-bbe7-2603bee338e8");

  ASSERT_L4OK(
    cmdline()->register_option("foo", Manager::No_arg, callback))
    << "A long option format can be registred.";
}

/// A nullptr cannot be registered as callback.
TEST(CmdlineOptionsCallback, Nullptr)
{
  TAP_UUID("e9906437-08a1-42b2-9184-a4689080846f");

  ASSERT_L4ERR(L4_EINVAL, cmdline()->register_option("null", Manager::No_arg,
                                                     nullptr))
    << "A long option format cannot be registred without a callback.";
}

/// option.name must be unique in the registry.
TEST(CmdlineOptionsBad, Options)
{
  TAP_UUID("0efc1fc3-f6c5-4806-81bf-432473fe76e6");

  EXPECT_L4OK(
    cmdline()->register_option("bar", Manager::No_arg, callback))
    << "A unique name and val can be registered.";

  EXPECT_L4ERR(L4_EINVAL,
    cmdline()->register_option("bar", Manager::No_arg, callback))
    << "Cannot register the same name twice.";
}

}} // namespace Tests::Atkins
