/*
 * Copyright (C) 2015-2016, 2019, 2024 Kernkonzept GmbH.
 * Author(s): Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include "debug.h"

namespace Vmm {

/**
 * A simple console output buffer to be used with early print
 * implementations via hypcall.
 */
class Guest_print_buffer
{
public:

  Guest_print_buffer()
  : _early_print_pos(0)
  {
    _early_print_buf[255] = '\0';
  }

  void print_char(char c)
  {
    if (!(c == '\n' || c == '\0'))
      _early_print_buf[_early_print_pos++] = c;

    if (_early_print_pos >= 255 || c == '\n' || c == '\0')
      {
        _early_print_buf[_early_print_pos] = '\0';
        printf("GUEST: %s\n", _early_print_buf);
        _early_print_pos = 0;
      }
  }

private:
  char _early_print_buf[256];
  unsigned _early_print_pos;
};

} // namespace
