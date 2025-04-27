/*
 * Copyright (C) 2016-2017, 2019, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Manuel von Oltersdorff-Kalettka <manuel.kalettka@kernkonzept.de>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */

#include <getopt.h>
#include <stdlib.h>
#include <cstring>
#include <type_traits>

#include <l4/cxx/exceptions>
#include <l4/re/error_helper>
#include <l4/re/env>

#include "debug.h"
#include "options.h"

bool
parse_int_optstring(char const *optstring, int *out)
{
  char *endp;

  errno = 0;
  long num = strtol(optstring, &endp, 10);

  // check that long can be converted to int
  if (errno || *endp != '\0' || num < INT_MIN || num > INT_MAX)
    return false;

  *out = num;

  return true;
}

static int
verbosity_mask_from_string(char const *str, unsigned *mask)
{
  if (strcmp("quiet", str) == 0)
    {
      *mask = Dbg::Quiet;
      return 0;
    }
  if (strcmp("warn", str) == 0)
    {
      *mask = Dbg::Warn;
      return 0;
    }
  if (strcmp("info", str) == 0)
    {
      *mask = Dbg::Warn | Dbg::Info;
      return 0;
    }
  if (strcmp("debug", str) == 0)
    {
      *mask = Dbg::Warn | Dbg::Info | Dbg::Debug;
      return 0;
    }
  if (strcmp("trace", str) == 0)
    {
      *mask = Dbg::Warn | Dbg::Info | Dbg::Debug | Dbg::Trace;
      return 0;
    }

  return -L4_ENOENT;
}

/**
 * Set debug level according to a verbosity string.
 *
 * The string may either set a global verbosity level:
 *   quiet, warn, info, trace
 *
 * Or it may set the verbosity level for a component:
 *
 *   <component>=<level>
 *
 * where component is one of: guest, core, cpu, mmio, irq, dev
 * and level the same as above.
 *
 * To change the verbosity of multiple components repeat
 * the verbosity switch.
 *
 * Example:
 *
 *  <program name> -D info -D port=trace
 *
 *    Sets verbosity for all components to info except for
 *    port handling which is set to trace.
 *
 *  <program name> -D trace -D port=warn -D queue=warn
 *
 *    Enables tracing for all components except port
 *    and queue.
 *
 */
static void
set_verbosity(char const *str)
{
  unsigned mask;
  if (verbosity_mask_from_string(str, &mask) == 0)
    {
      Dbg::set_verbosity(mask);
      return;
    }

  static char const *const components[] =
    { "core", "virtio", "port", "request", "queue", "packet" };

  static_assert(std::extent<decltype(components)>::value == Dbg::Max_component,
                "Component names must match 'enum Component'.");

  for (unsigned i = 0; i < Dbg::Max_component; ++i)
    {
      auto len = strlen(components[i]);
      if (strncmp(components[i], str, len) == 0 && str[len] == '='
          && verbosity_mask_from_string(str + len + 1, &mask) == 0)
        {
          Dbg::set_verbosity(i, mask);
          return;
        }
    }
}

int
Options::parse_cmd_line(int argc, char **argv,
                        std::shared_ptr<Ds_vector> trusted_dataspaces)
{
  int opt, index;

  struct option options[] =
    {
      {"size",        1, 0, 's' }, // size of in/out queue == #buffers in queue
      {"ports",       1, 0, 'p' }, // number of ports
      {"mac",         0, 0, 'm' }, // switch sets MAC address for each client
      {"debug",       1, 0, 'D' }, // configure debug levels
      {"verbose",     0, 0, 'v' },
      {"quiet",       0, 0, 'q' },
      {"register-ds", 1, 0, 'd' }, // register a trusted dataspace
      {0, 0, 0, 0}
    };

  unsigned long verbosity = Dbg::Warn;

  Dbg info(Dbg::Core, Dbg::Info);

  Dbg::set_verbosity(Dbg::Core, Dbg::Info);
  info.printf("Arguments:\n");
  for (int i = 0; i < argc; ++i)
    info.printf("\t%s\n", argv[i]);

  Dbg::set_verbosity(verbosity);
  while ( (opt = getopt_long(argc, argv, "s:p:mqvD:d:", options, &index)) != -1)
    {
      switch (opt)
        {
        case 's':

          // QueueNumMax must be power of 2 between 1 and 0x8000
          if (!parse_int_optstring(optarg, &_virtq_max_num)
              || _virtq_max_num < 1 || _virtq_max_num > 32768
              || (_virtq_max_num & (_virtq_max_num - 1)))
            {
              Err().printf("Max number of virtqueue buffers must be power of 2"
                           " between 1 and 32768. Invalid value %i or argument "
                           "%s\n",
                           _virtq_max_num, optarg);
              return -1;
            }
          info.printf("Max number of buffers in virtqueue: %i\n",
                      _virtq_max_num);
          break;
        case 'p':
          if (parse_int_optstring(optarg, &_max_ports))
            info.printf("Max number of ports: %u\n", _max_ports);
          else
            {
              Err().printf("Invalid number of ports argument: %s\n", optarg);
              return -1;
            }
          break;
        case 'q':
          verbosity = Dbg::Quiet;
          Dbg::set_verbosity(verbosity);
          break;
        case 'v':
          verbosity = (verbosity << 1) | 1;
          Dbg::set_verbosity(verbosity);
          break;
        case 'D':
          set_verbosity(optarg);
          break;
        case 'm':
          info.printf("Assigning mac addresses\n");
          _assign_mac = true;
          break;
        case 'd':
          {
            L4::Cap<L4Re::Dataspace> ds =
              L4Re::chkcap(L4Re::Env::env()->get_cap<L4Re::Dataspace>(optarg),
                           "Find a dataspace capability.\n");
            trusted_dataspaces->push_back(ds);
            break;
          }
        default:
          Err().printf("Unknown command line option '%c' (%d)\n", opt, opt);
          return -1;
        }
    }
  return 0;
}

static Options options;

Options const *
Options::get_options()
{ return &options; }

Options const *
Options::parse_options(int argc, char **argv,
                       std::shared_ptr<Ds_vector> trusted_dataspaces)
{
  if (options.parse_cmd_line(argc, argv, trusted_dataspaces) < 0)
    return nullptr;

  return &options;
}
