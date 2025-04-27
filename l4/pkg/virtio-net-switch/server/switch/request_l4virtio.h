/*
 * Copyright (C) 2016-2017, 2020, 2022, 2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include "debug.h"
#include "port.h"
#include "request.h"
#include "virtio_net.h"

#include <l4/l4virtio/server/virtio>
#include <l4/util/assert.h>

#include <optional>
#include <utility>

/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * Abstraction for a network request
 *
 * A `Virtio_net_request` is constructed by the source port, using the static
 * function `get_request()` as part of `Port_iface::get_tx_request()`.
 *
 * On destruction, `finish()` will be called, which, will trigger the client
 * IRQ of the source client.
 */
class Virtio_net_request final : public Net_request
{
public:
  class Virtio_net_transfer final : public Net_transfer
  {
  public:
    explicit Virtio_net_transfer(Virtio_net_request const &request)
    : _request(request),
      // We already looked at the very first buffer to find the target of the
      // packet. The request processor of the "parent request" contains the
      // current state of the transaction up to this point. Since there might be
      // more then one target for the request we have to keep track of our own
      // state and need our own request processor instance, which will be
      // initialized using the current state of the "parent request".
      _req_proc(_request.get_request_processor())
    {
      // The buffer descriptors used for this transaction and the amount of bytes
      // copied to the current target descriptor.
      _cur_buf = request.first_buffer();
      _req_id = _request.header();
    }

    // delete copy constructor and copy assignment operator
    Virtio_net_transfer(Virtio_net_transfer const &) = delete;
    Virtio_net_transfer &operator = (Virtio_net_transfer const &) = delete;

    void copy_header(Virtio_net::Hdr *dst_header) const override
    {
      memcpy(dst_header, _request.header(), sizeof(Virtio_net::Hdr));
    }

    bool done() override
    {
      return _cur_buf.done() && !_req_proc.next(_request.dev()->mem_info(), &_cur_buf);
    }

  private:
    Virtio_net_request const &_request;
    L4virtio::Svr::Request_processor _req_proc;
  };

  void dump_request(Port_iface *port) const
  {
    Dbg debug(Dbg::Request, Dbg::Debug, "REQ-VIO");
    if (debug.is_active())
      {
        debug.printf("%s: Next packet: %p:%p - %x bytes\n",
                     port->get_name(), _header, _pkt.pos, _pkt.left);
        if (_header->flags.raw || _header->gso_type)
          {
            debug.cprintf("flags:\t%x\n\t"
                          "gso_type:\t%x\n\t"
                          "header len:\t%x\n\t"
                          "gso size:\t%x\n\t"
                          "csum start:\t%x\n\t"
                          "csum offset:\t%x\n"
                          "\tnum buffer:\t%x\n",
                          _header->flags.raw,
                          _header->gso_type, _header->hdr_len,
                          _header->gso_size,
                          _header->csum_start, _header->csum_offset,
                          _header->num_buffers);
          }
      }
    dump_pkt();
  }

  // delete copy constructor and copy assignment operator
  Virtio_net_request(Virtio_net_request const &) = delete;
  Virtio_net_request &operator = (Virtio_net_request const &) = delete;

  // define move constructor and copy assignment operator
  Virtio_net_request(Virtio_net_request &&other)
  : _dev(other._dev),
    _queue(other._queue),
    _head(std::move(other._head)),
    _req_proc(std::move(other._req_proc)),
    _header(other._header)
  {
    _pkt = std::move(other._pkt);

    // Invalidate other.
    other._queue = nullptr;
  }

  Virtio_net_request &operator = (Virtio_net_request &&other)
  {
    // Invalidate self.
    finish();

    _dev = other._dev;
    _queue = other._queue;
    _head = std::move(other._head);
    _req_proc = std::move(other._req_proc);
    _header = other._header;
    _pkt = std::move(other._pkt);

    // Invalidate other.
    other._queue = nullptr;

    return *this;
  }

  Virtio_net_request(Virtio_net *dev, L4virtio::Svr::Virtqueue *queue,
                     L4virtio::Svr::Virtqueue::Request const &req)
  : _dev(dev), _queue(queue)
  {
    _head = _req_proc.start(_dev->mem_info(), req, &_pkt);

    _header = (Virtio_net::Hdr *)_pkt.pos;
    l4_uint32_t skipped = _pkt.skip(sizeof(Virtio_net::Hdr));

    if (L4_UNLIKELY(   (skipped != sizeof(Virtio_net::Hdr))
                    || (_pkt.done() && !_next_buffer(&_pkt))))
      {
        _header = 0;
        Dbg(Dbg::Queue, Dbg::Warn).printf("Invalid request\n");
        return;
      }
  }

  ~Virtio_net_request()
  { finish(); }

  bool valid() const
  { return _header != 0; }

  /**
   * Drop all requests of a specific queue.
   *
   * This function is used for example to drop all requests in the transmission
   * queue of a monitor port, since monitor ports are not allowed to transmit
   * data.
   *
   * \param dev    Port of the provided virtqueue.
   * \param queue  Virtqueue to drop all requests of.
   */
  static void drop_requests(Virtio_net *dev,
                            L4virtio::Svr::Virtqueue *queue)
  {
    if (L4_UNLIKELY(!queue->ready()))
      return;

    if (queue->desc_avail())
      Dbg(Dbg::Request, Dbg::Debug)
        .printf("Dropping incoming packets on monitor port\n");

    L4virtio::Svr::Request_processor req_proc;
    Buffer pkt;

    while (auto req = queue->next_avail())
      {
        auto head = req_proc.start(dev->mem_info(), req, &pkt);
        queue->finish(head, dev, 0);
      }
  }

  /**
   * Construct a request from the next entry of a provided queue.
   *
   * \param dev    Port of the provided virtqueue.
   * \param queue  Virtqueue to extract next entry from.
   */
  static std::optional<Virtio_net_request>
  get_request(Virtio_net *dev, L4virtio::Svr::Virtqueue *queue)
  {
    if (L4_UNLIKELY(!queue->ready()))
      return std::nullopt;

    if (auto r = queue->next_avail())
      {
        // Virtio_net_request keeps "a lot of internal state",
        // therefore we create the object before creating the
        // state.
        // We might check later on whether it is possible to
        // save the state when we actually have to because a
        // transfer is blocking on a port.
        auto request = Virtio_net_request(dev, queue, r);
        if (request.valid())
          return request;
      }
    return std::nullopt;
  }

  Buffer const &first_buffer() const
  { return _pkt; }

  Virtio_net::Hdr const *header() const
  { return _header; }

  L4virtio::Svr::Request_processor const &get_request_processor() const
  { return _req_proc; }

  Virtio_net const *dev() const
  { return _dev; }

  Virtio_net_transfer transfer_src() const
  { return Virtio_net_transfer(*this); }

private:
  /* needed for Virtqueue::finish() */
  /** Source Port */
  Virtio_net *_dev;
  /** transmission queue of the source port */
  L4virtio::Svr::Virtqueue *_queue;
  L4virtio::Svr::Virtqueue::Head_desc _head;

  /* the actual request processor, encapsulates the decoding of the request */
  L4virtio::Svr::Request_processor _req_proc;

  /* A request to the virtio net layer consists of one or more buffers
     containing the Virtio_net::Hdr and the actual packet. To make a
     switching decision we need to be able to look at the packet while
     still being able access the Virtio_net::Hdr for the actual copy
     operation. Therefore we keep track of two locations, the header
     location and the start of the packet (which might be in a
     different buffer) */
  Virtio_net::Hdr *_header;

  bool _next_buffer(Buffer *buf)
  { return _req_proc.next(_dev->mem_info(), buf); }

  /**
   * Finalize request
   *
   * This function calls `finish()` on the source port's transmission queue,
   * which will result in triggering the source client IRQ.
   */
  void finish()
  {
    if (_queue == nullptr || !_queue->ready())
      return;

    Dbg(Dbg::Virtio, Dbg::Trace).printf("%s(%p)\n", __PRETTY_FUNCTION__, this);
    _queue->finish(_head, _dev, 0);
    _queue = nullptr;
  }
};

/**\}*/
