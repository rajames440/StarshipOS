/*
 * Copyright (C) 2024 Kernkonzept GmbH.
 * Author(s): Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include "port.h"
#include "request.h"

#include <l4/ixl/memory.h>

#include <utility>

/**
 * \ingroup virtio_net_switch
 * \{
 */

class Ixl_net_request final : public Net_request
{
public:
  class Ixl_net_transfer final : public Net_transfer
  {
  public:
    explicit Ixl_net_transfer(Ixl_net_request const &request)
    : _request(request)
    {
      _cur_buf = Buffer(reinterpret_cast<char *>(request.buf()->data),
                        request.buf()->size);
      _req_id = _request.buf();
    }

    // delete copy constructor and copy assignment operator
    Ixl_net_transfer(Ixl_net_transfer const &) = delete;
    Ixl_net_transfer &operator = (Ixl_net_transfer const &) = delete;

    void copy_header(Virtio_net::Hdr *dst_header) const override
    {
      dst_header->flags.data_valid() = 0;
      dst_header->flags.need_csum() = 0;
      dst_header->gso_type = 0; // GSO_NONE
      dst_header->hdr_len = sizeof(Virtio_net::Hdr);
      dst_header->gso_size = 0;
      dst_header->csum_start = 0;
      dst_header->csum_offset = 0;
      dst_header->num_buffers = 1;
    }

    bool done() override { return _cur_buf.done(); }

  private:
    Ixl_net_request const &_request;
  };

  void dump_request(Port_iface *port) const
  {
    Dbg debug(Dbg::Request, Dbg::Debug, "REQ-IXL");
    if (debug.is_active())
      {
        debug.printf("%s: Next packet: %p - %x bytes\n",
                     port->get_name(), _pkt.pos, _pkt.left);
      }
    dump_pkt();
  }

  explicit Ixl_net_request(Ixl::pkt_buf *buf) : _buf(buf)
  {
    _pkt = Buffer(reinterpret_cast<char *>(buf->data), buf->size);
  }

  // delete copy constructor and copy assignment operator
  Ixl_net_request(Ixl_net_request const &) = delete;
  Ixl_net_request &operator=(Ixl_net_request const &) = delete;

  // define move constructor and copy assignment operator
  Ixl_net_request(Ixl_net_request &&other)
  : _buf(other._buf)
  {
    _pkt = std::move(other._pkt);

    // Invalidate other.
    other._buf = nullptr;
  }

  Ixl_net_request &operator=(Ixl_net_request &&other)
  {
    // Invalidate self.
    if (_buf != nullptr)
      Ixl::pkt_buf_free(_buf);

    _buf = other._buf;
    _pkt = std::move(other._pkt);

    // Invalidate other.
    other._buf = nullptr;

    return *this;
  }

  ~Ixl_net_request()
  {
    if (_buf != nullptr)
      {
        Ixl::pkt_buf_free(_buf);
        _buf = nullptr;
      }
  }

  Ixl::pkt_buf *buf() const { return _buf; }

  Ixl_net_transfer transfer_src() const
  { return Ixl_net_transfer(*this); }

private:
  Ixl::pkt_buf *_buf;
};

/**\}*/
