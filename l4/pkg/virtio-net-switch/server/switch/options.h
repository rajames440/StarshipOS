/*
 * Copyright (C) 2016-2017, 2022, 2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Manuel von Oltersdorff-Kalettka <manuel.kalettka@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <memory>
#include <vector>
#include <cerrno>
#include <climits>

#include <l4/re/dataspace>

bool
parse_int_optstring(char const *optstring, int *out);

class Options
{
  using Ds_vector = std::vector<L4::Cap<L4Re::Dataspace>>;
public:
  int get_max_ports() const
  { return _max_ports; }

  int get_virtq_max_num() const
  { return _virtq_max_num; }

  int get_portq_max_num() const
  { return _portq_max_num; }

  int get_request_timeout() const
  { return _request_timeout; }

  int assign_mac() const
  { return _assign_mac; }

  static Options const *
  parse_options(int argc, char **argv,
                std::shared_ptr<Ds_vector> trusted_dataspaces);
  static Options const *get_options();

private:
  int _max_ports = 5;
  int _virtq_max_num = 0x100; // default value for data queues
  int _portq_max_num = 50;    // default value for port queues
  int _request_timeout = 1 * 1000 * 1000; // default packet timeout 1 second
  bool _assign_mac = false;

  int parse_cmd_line(int argc, char **argv,
                     std::shared_ptr<Ds_vector> trusted_dataspaces);
};
