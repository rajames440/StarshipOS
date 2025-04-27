/*
 * Copyright (C) 2024 Kernkonzept GmbH.
 * Author(s): Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include "port.h"
#include "request_ixl.h"

#include <l4/ixl/device.h>
#include <l4/ixl/memory.h>

#include <optional>

/**
 * \ingroup virtio_net_switch
 * \{
 */

class Ixl_port : public Port_iface
{
public:
  static constexpr unsigned Tx_batch_size = 32;
  static constexpr unsigned Num_bufs = 1024;
  static constexpr unsigned Buf_size = 2048;
  static constexpr l4_uint64_t Max_mem_size = 1ULL << 28;

  Ixl_port(Ixl::Ixl_device *dev)
  : Port_iface(dev->get_driver_name().c_str()),
    _dev(dev),
    _mempool(*_dev, Num_bufs, Buf_size, Max_mem_size)
  {
    Ixl::mac_address mac_addr = _dev->get_mac_addr();
    _mac = Mac_addr(reinterpret_cast<char const *>(mac_addr.addr));
  }

  // OPTIMIZE: Could use this information for rx batching, i.e. collect while
  //           rx_notify is disabled, then flush the collected buffers when
  //           rx_notify is enabled again.
  void rx_notify_disable_and_remember() override {}
  void rx_notify_emit_and_enable() override {}
  bool is_gone() const override { return false; }

  /** Check whether there is any work pending on the transmission queue */
  bool tx_work_pending()
  {
    fetch_tx_requests();
    return _tx_batch_idx < _tx_batch_len;
  }

  /** Get one request from the transmission queue */
  std::optional<Ixl_net_request> get_tx_request()
  {
    fetch_tx_requests();
    if (_tx_batch_idx < _tx_batch_len)
      return std::make_optional<Ixl_net_request>(_tx_batch[_tx_batch_idx++]);
    else
      return std::nullopt;
  }

  Result handle_request(Port_iface *src_port, Net_transfer &src) override
  {
    Virtio_vlan_mangle mangle = create_vlan_mangle(src_port);

    Dbg trace(Dbg::Request, Dbg::Trace, "REQ-IXL");
    trace.printf("%s: Transfer request %p.\n", _name, src.req_id());

    struct Ixl::pkt_buf *buf = _mempool.pkt_buf_alloc();
    if (!buf)
      {
        trace.printf("\tTransfer failed, out-of-memory, dropping.\n");
        return Result::Dropped;
      }

    // NOTE: Currently, the switch does not offer checksum or segmentation
    //       offloading to its l4virtio clients, so it is fine to simply ignore
    //       the Virtio_net::Hdr of the request here.

    // Copy the request to the pkt_buf.
    Buffer dst_buf(reinterpret_cast<char *>(buf->data),
                   Buf_size - offsetof(Ixl::pkt_buf, data));
    unsigned max_size = Buf_size - offsetof(Ixl::pkt_buf, data);
    for (;;)
      {
        try
          {
            if (src.done())
              // Request completely copied to destination.
              break;
          }
        catch (L4virtio::Svr::Bad_descriptor &e)
          {
            trace.printf("\tTransfer failed, bad descriptor exception, dropping.\n");

            // Handle partial transfers to destination port.
            Ixl::pkt_buf_free(buf);
            throw;
          }

        if (dst_buf.done())
          {
            trace.printf(
              "\tTransfer failed, exceeds max packet-size, dropping.\n");
            Ixl::pkt_buf_free(buf);
            return Result::Dropped;
          }

        auto &src_buf = src.cur_buf();
        trace.printf("\tCopying %p#%p:%u (%x) -> %p#%p:%u  (%x)\n",
                     src_port, src_buf.pos, src_buf.left, src_buf.left,
                     static_cast<Port_iface *>(this),
                     dst_buf.pos, dst_buf.left, dst_buf.left);

        mangle.copy_pkt(dst_buf, src_buf);
      }
    buf->size = max_size - dst_buf.left;

    // Enqueue the pkt_buf at the device.
    if (_dev->tx_batch(0, &buf, 1) == 1)
      {
        trace.printf("\tTransfer queued at device.\n");
        return Result::Delivered;
      }
    else
      {
        trace.printf("\tTransfer failed, dropping.\n");
        Ixl::pkt_buf_free(buf);
        return Result::Dropped;
      }
  }

  Ixl::Ixl_device *dev() { return _dev; }

private:
  void fetch_tx_requests()
  {
    if (_tx_batch_idx < _tx_batch_len)
      // Previous batch not yet fully processed.
      return;

    // Batch receive, then cache in member array, to avoid frequent interactions
    // with the hardware.
    _tx_batch_len = _dev->rx_batch(0, _tx_batch, Tx_batch_size);
    _tx_batch_idx = 0;
  }

  Ixl::Ixl_device *_dev;
  Ixl::Mempool _mempool;
  Ixl::pkt_buf *_tx_batch[Tx_batch_size];
  unsigned _tx_batch_idx = 0;
  unsigned _tx_batch_len = 0;
};

/**\}*/
