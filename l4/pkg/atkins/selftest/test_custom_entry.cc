/* SPDX-License-Identifier: GPL-2.0-only or License-Ref-kk-custom */
/*
 * Copyright (C) 2019, 2022 Kernkonzept GmbH.
 * Author(s): Andreas Otto <andreas.otto@kernkonzept.com>
 */

/**
 * Tests whether the mechanism to parse custom arguments with tap/cmdline
 * works correctly.
 */

#include <l4/atkins/app_runner>
#include <l4/atkins/factory>
#include <l4/atkins/debug>
#include <l4/atkins/l4_assert>
#include <l4/atkins/tap/cmdline>
#include <l4/atkins/tap/tap>
#include <l4/atkins/tap/cov>
#include <l4/re/env>

#include <make_unique-l4>
#include <terminate_handler-l4>

using Atkins::Ipc_helper::Default_test_timeout;

/**
 * The custom entry is called correctly and sends its argument as a message tag.
 */
TEST(CustomEntry, SendPing)
{
  TAP_UUID("755a2eb2-7ce4-49ae-9a7c-3d7f38d72c80");

  enum { Magic_label = 0x420, Magic_tag = 0x31 };
  auto gate = Atkins::Factory::kobj<L4::Ipc_gate>("Create IPC gate.");
  gate->bind_thread(L4Re::Env::env()->main_thread(), Magic_label);

  auto app = std::L4::make_unique<Atkins::App_runner_with_exit_handler>(
    "rom/test_custom_entry");

  char buf[10];
  snprintf(buf, 10, "-e%d", Magic_tag);
  app->append_cmdline("-vv");
  app->append_cmdline(buf);
  app->add_initial_cap("gate", gate.get(), L4_CAP_FPAGE_RW);
  app->exec();

  l4_umword_t label;
  auto tag = L4Re::chkipc(l4_ipc_wait(l4_utcb(), &label, Default_test_timeout),
                          "Receive ping from helper task.");
  EXPECT_EQ(Magic_label, label & ~(L4_CAP_FPAGE_W | L4_CAP_FPAGE_S))
    << "Ping received through IPC gate.";
  EXPECT_EQ(Magic_tag, tag.label()) << "Helper task sent expected tag.";

  L4Re::chkipc(app->wait_for_exit(), "Wait for helper task exit.");
}

static void custom_entry(int tag)
{
  auto gate = L4Re::chkcap(L4Re::Env::env()->get_cap<L4::Ipc_gate>("gate"),
                          "Get IPC gate cap from initial caps.");
  L4Re::chkipc(l4_ipc_send(gate.cap(), l4_utcb(), l4_msgtag(tag, 0, 0, 0),
                           Default_test_timeout),
               "Send ping to main task.");
}

static int set_g_tag = false;
static int g_tag;

inline void
callback(char const *tag)
{
  using Atkins::Dbg;

  Dbg(Dbg::Info).printf("callback setting tag %s\n", tag);
  g_tag = std::stoi(tag);
  set_g_tag = true;
}

/**
 * Singleton needed to manage the cmdline.
 *
 * Ensure it is only used once per .t-file!
 */
static Atkins::Cmdline::Manager *
Atkins::Cmdline::cmdline()
{
  static Manager cmd;
  return &cmd;
}

using namespace Atkins::Cmdline;

GTEST_API_ int
main(int argc, char **argv)
{
  using Atkins::Dbg;

  testing::InitGoogleTest(&argc, argv);
  //
  // Delete the default listener.
  testing::TestEventListeners &listeners =
    testing::UnitTest::GetInstance()->listeners();
  delete listeners.Release(listeners.default_result_printer());

  // Create before the cmdline parser is invoked.
  listeners.Append(new Atkins::Tap::Tap_listener());

  L4Re::chksys(cmdline()->register_flag('e', Manager::Req_arg, callback),
               "Cannot register 'e:' flag with cmdline\n");

  cmdline()->parse(argc, argv);

  Dbg(Dbg::Info).printf("Parsed options Atkins\n");

  listeners.Append(new Atkins::Cov::Cov_listener());

  l4_debugger_set_object_name(L4Re::Env::env()->main_thread().cap(),
                              "custom_main");

  int ret = 0;

  if (set_g_tag)
    custom_entry(g_tag);
  else
    ret = RUN_ALL_TESTS();

  return ret;
}
