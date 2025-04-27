/*
 * Copyright (C) 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "drv.h"

namespace Emmc {

void
Drv_base::delay(unsigned ms)
{
  stats_wait_start();
  l4_ipc_sleep_ms(ms);
  stats_wait_done();
}

}
