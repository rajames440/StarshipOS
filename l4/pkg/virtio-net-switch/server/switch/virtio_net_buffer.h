/*
 * Copyright (C) 2016-2017, 2022, 2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/l4virtio/server/l4virtio>
/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * Data buffer used to transfer packets.
 */
struct Buffer : L4virtio::Svr::Data_buffer
{
  Buffer() = default;
  Buffer(L4virtio::Svr::Driver_mem_region const *r,
         L4virtio::Svr::Virtqueue::Desc const &d,
         L4virtio::Svr::Request_processor const *)
  {
    pos = static_cast<char *>(r->local(d.addr));
    left = d.len;
  }

  Buffer(char *data, l4_uint32_t size)
  {
    pos = data;
    left = size;
  }

  template<typename T>
  explicit Buffer(T *p) : Data_buffer(p) {};
};

/**\}*/
