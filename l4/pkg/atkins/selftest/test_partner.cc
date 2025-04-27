/* SPDX-License-Identifier: GPL-2.0-only or License-Ref-kk-custom */
/*
 * Copyright (C) 2021-2023 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 */

/*
 * This self test checks the basic functionality of the partner
 * abstraction. It provides a test case for both the direct
 * interaction and the interaction over a gate.
 *
 * It can serve as a template for tests which need interactions
 * between two partners.
 */
#include <l4/atkins/l4_assert>
#include <l4/atkins/tap/main_helper>
#include <l4/atkins/thread_helper>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>

// Declare relevant name spaces
namespace Tests { namespace Atkins_lib {

using Atkins::Partner;
using Atkins::Partner_info;
using Atkins::Ipc_helper::Default_test_timeout;

/**
 * Declare a test class with (at least) two booleans as parameters
 */
struct PartnerTest : public testing::TestWithParam<std::tuple<bool,bool>>
{
  void CheckCores()
  {
    std::tie(task, cross_cpu) = GetParam();
    if (cross_cpu && Partner::online_cores() < 2)
      SKIP("Enable more cores to run this test.");
  }
  bool cross_cpu;
  bool task;
};

/**
 * Declare test suite (test group, class name, parameters, parameter
 * to string function)
 */
static INSTANTIATE_TEST_SUITE_P(
  PartnerTests, PartnerTest,
  testing::Combine(testing::Bool(),testing::Bool()),
  [](testing::TestParamInfo<PartnerTest::ParamType> const &t)
  {
    std::string res = std::get<0>(t.param) ? "SeperateSpace" : "SharedSpace";
    res.append(std::get<1>(t.param) ? "Cross" : "Single");
    return res;
  });

/**
 * Example for a test with direct thread communication
 */
/// Helper function for direct thread interaction
static void
thread_helper(Partner_info const &partner)
{
  /**
   * partner
   *   .partner - capability of partner thread
   *   .gate    - (optional) capability of gate
   *   .task    - true if test runs in seperate task
   */
  // Do something with the partner, e.g. send a message to the partner
  L4Re::chkipc(l4_ipc_send(partner.partner.cap(), l4_utcb(),
                           l4_msgtag(0, 0, 0, 0),
                           Default_test_timeout),
               "Send msg to main thread");

}
/**
 * Register Test, name needs to be unique in the set of tests in this
 * file.
 */
static char const * const thread_helper_name = "ThreadTest";
static Partner::Test_entry e(thread_helper_name, thread_helper);

/// Actual test
TEST_P(PartnerTest, Thread)
{
  TAP_UUID("5e4dc11a-3cd4-41a8-a60a-d2a36cbb6b2e");

  CheckCores();

  // Instantiate a partner object without a gate and start the partner
  Partner p;
  p.start(thread_helper_name, task, cross_cpu);

  // Do something with the partner, e.g receive a message
  ASSERT_L4IPC_OK(l4_ipc_receive(p.partner_cap().cap(), l4_utcb(),
                                 Default_test_timeout))
    << "Receive IPC from the partner thread.";
}


/**
 * Example for a test with communication over a gate
 */

/// Dummy handler used for object registry, use your own class as needed
struct Null_handler : L4::Epiface_t<Null_handler, L4::Kobject>
{};

/// Helper function for communication over a gate
static void
gate_helper(Partner_info const &partner)
{
  /**
   * partner
   *   .partner - capability of partner thread
   *   .gate    - (optional) capability of gate
   *   .task    - true if test runs in seperate task
   */
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_timeout_hooks>
    server(Atkins::Thread_helper::this_thread_cap(),
           L4Re::Env::env()->factory());
  struct Null_handler dummy;

  server.registry()->register_obj(&dummy, partner.gate);

  l4_umword_t recv_label;
  L4Re::chksys(l4_ipc_wait(l4_utcb(), &recv_label, Default_test_timeout),
               "Wait for notification from partner thread");
}
/*
 * Register Test, name needs to be unique in the set of tests in this
 * file.
 */
static char const * const gate_helper_name = "GateTest";
static Partner::Test_entry e1(gate_helper_name, gate_helper);

/// Actual test
TEST_P(PartnerTest, Gate)
{
  TAP_UUID("52982c71-a429-4498-97bf-1d96e1c88441");

  CheckCores();

  // Instantiate a partner object with a gate and start the partner
  Partner p(true);
  p.start(gate_helper_name, task, cross_cpu);

  // Send a message to the gate
  ASSERT_L4IPC_OK(l4_ipc_send(p.gate().cap(), l4_utcb(),
                              l4_msgtag(0, 0, 0, 0),
                              Default_test_timeout))
    << "Send a message to the gate.";
}
}} // namespace Fiasco, namespace Tests
