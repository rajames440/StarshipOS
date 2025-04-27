/*
 * Copyright (C) 2020-2021, 2023-2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *            Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#pragma once

#include <l4/re/util/debug>
#include <cstdio>

struct Err : L4Re::Util::Err
{
  explicit Err(Level l = Normal) : L4Re::Util::Err(l, "eMMC") {}
};

class Dbg : public L4Re::Util::Dbg
{
public:
  enum Level
  {
    Warn   = 1,
    Info   = 2,
    Trace  = 4,
    Trace2 = 8
  };

  Dbg(unsigned long l = Info, char const *subsys = nullptr, int nr = -1)
  : L4Re::Util::Dbg(l, create_comp_str("eMMC", nr), subsys) {}

  static Dbg warn(char const *subsys = nullptr)
  { return Dbg(Dbg::Warn, subsys); }

  static Dbg info(char const *subsys = nullptr)
  { return Dbg(Dbg::Info, subsys); }

  static Dbg trace(char const *subsys = nullptr)
  { return Dbg(Dbg::Trace, subsys); }

  static Dbg trace2(char const *subsys = nullptr)
  { return Dbg(Dbg::Trace2, subsys); }

private:
  char const *create_comp_str(char const *const comp, int nr)
  {
    if (nr == -1)
      return comp;
    snprintf(comp_str, sizeof(comp_str), "%s-%d", comp, nr);
    return comp_str;
  }

  char comp_str[32];
};
