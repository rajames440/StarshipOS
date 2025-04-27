/*
 * Copyright (C) 2016-2017, 2023-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include "request.h"
#include <l4/bid_config.h>

/**
 * Decide whether a packet shall be filtered out.
 *
 * \param buf   The buffer containing the available part of the package.
 * \param size  The size of the available packet data.
 *
 * \retval true   The packet shall be filtered out.
 * \retval false  The packet shall be forwarded.
 *
 * This function looks at the available part of a packet and decides
 * whether it shall be filtered.
*/
#ifdef CONFIG_VNS_PORT_FILTER
bool filter(const uint8_t *buf, size_t size);
#else
inline bool filter(const uint8_t *, size_t)
{
  // default implementation filtering out no packets, see filter.cc for
  // other examples
  return false;
}
#endif

/**
 * Look at a request and decide whether it shall be filtered.
 *
 * \param req  The request to be considered for filtering.
 *
 * \retval true   The packet shall be filtered out.
 * \retval false  The packet shall be forwarded.
*/
inline bool filter_request(Net_request const &req)
{
  size_t size;
  const uint8_t *buf = req.buffer(&size);
  return filter(buf, size);
}
