/*
 * Copyright (C) 2016-2017, 2020, 2022-2024 Kernkonzept GmbH.
 * Author(s): Jean Wolter <jean.wolter@kernkonzept.com>
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *            Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 *
 * This file is distributed under the terms of the GNU General Public
 * License, version 2.  Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include "port.h"
#include "request_l4virtio.h"
#include "virtio_net.h"

#include <l4/cxx/pair>

#include <vector>

/**
 * \ingroup virtio_net_switch
 * \{
 */

/**
 * A Port on the Virtio Net Switch
 *
 * A Port object gets created by `Virtio_factory::op_create()`. This function
 * actually only instantiates objects of the types `Switch_port` and
 * `Monitor_port`. The created Port registers itself at the switch's server.
 * Usually, the IPC call for port creation comes from ned. To finalize the
 * setup, the client has to initialize the port during the virtio
 * initialization phase. To do this, the client registers a dataspace for
 * queues and buffers and provides an IRQ to notify the client on incoming
 * network requests.
 */
class L4virtio_port : public Port_iface, public Virtio_net
{
public:
  /**
   * Create a Virtio net port object
   */
  explicit L4virtio_port(unsigned vq_max, unsigned num_ds, char const *name,
                         l4_uint8_t const *mac)
  : Port_iface(name), Virtio_net(vq_max)
  {
    init_mem_info(num_ds);

    Features hf = _dev_config.host_features(0);
    if (mac)
      {
        _mac = Mac_addr((char const *)mac);
        memcpy((void *)_dev_config.priv_config()->mac, mac,
               sizeof(_dev_config.priv_config()->mac));

        hf.mac() = true;
        Dbg d(Dbg::Port, Dbg::Info);
        d.cprintf("%s: Adding Mac '", _name);
        _mac.print(d);
        d.cprintf("' to host features to %x\n", hf.raw);
      }
    _dev_config.host_features(0) = hf.raw;
    _dev_config.reset_hdr();
    Dbg(Dbg::Port, Dbg::Info)
      .printf("%s: Set host features to %x\n", _name,
              _dev_config.host_features(0));
  }

  void rx_notify_disable_and_remember() override
  {
    kick_disable_and_remember();
  }

  void rx_notify_emit_and_enable() override
  {
    kick_emit_and_enable();
  }

  bool is_gone() const override
  {
    return obj_cap() && !obj_cap().validate().label();
  }

  /** Check whether there is any work pending on the transmission queue */
  bool tx_work_pending() const
  {
    return L4_LIKELY(tx_q()->ready()) && tx_q()->desc_avail();
  }

  /** Get one request from the transmission queue */
  std::optional<Virtio_net_request> get_tx_request()
  {
    return Virtio_net_request::get_request(this, tx_q());
  }

  /**
   * Drop all requests pending in the transmission queue.
   *
   * This is used for monitor ports, which are not allowed to send packets.
   */
  void drop_requests()
  { Virtio_net_request::drop_requests(this, tx_q()); }

  Result handle_request(Port_iface *src_port, Net_transfer &src) override
  {
    Virtio_vlan_mangle mangle = create_vlan_mangle(src_port);

    Dbg trace(Dbg::Request, Dbg::Trace, "REQ-VIO");
    trace.printf("%s: Transfer request %p.\n", _name, src.req_id());

    Buffer dst;
    int total = 0;
    l4_uint16_t num_merged = 0;
    typedef cxx::Pair<L4virtio::Svr::Virtqueue::Head_desc, l4_uint32_t> Consumed_entry;
    std::vector<Consumed_entry> consumed;

    Virtio_net *dst_dev = this;
    Virtqueue *dst_queue = rx_q();
    L4virtio::Svr::Virtqueue::Head_desc dst_head;
    L4virtio::Svr::Request_processor dst_req_proc;
    Virtio_net::Hdr *dst_header = nullptr;

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
            if (!consumed.empty())
              // Partial transfer, rewind to before first descriptor of transfer.
              dst_queue->rewind_avail(consumed.at(0).first);
            else if (dst_head)
              // Partial transfer, still at first _dst_head.
              dst_queue->rewind_avail(dst_head);
            throw;
          }

        /* The source data structures are already initialized, the header
           is consumed and src stands at the very first real buffer.
           Initialize the target data structures if necessary and fill the
           header. */
        if (!dst_head)
          {
            if (!dst_queue->ready())
              return Result::Dropped;

            auto r = dst_queue->next_avail();

            if (L4_UNLIKELY(!r))
              {
                trace.printf("\tTransfer failed, destination queue depleted, dropping.\n");
                // Abort incomplete transfer.
                if (!consumed.empty())
                  dst_queue->rewind_avail(consumed.front().first);
                return Result::Dropped;
              }

            try
              {
                dst_head = dst_req_proc.start(dst_dev->mem_info(), r, &dst);
              }
            catch (L4virtio::Svr::Bad_descriptor &e)
              {
                Dbg(Dbg::Request, Dbg::Warn, "REQ")
                  .printf("%s: bad descriptor exception: %s - %i"
                          " -- signal device error in destination device %p.\n",
                          __PRETTY_FUNCTION__, e.message(), e.error, dst_dev);

                dst_dev->device_error();
                return Result::Exception; // Must not touch the dst queues anymore.
              }

            if (!dst_header)
              {
                if (dst.left < sizeof(Virtio_net::Hdr))
                  throw L4::Runtime_error(-L4_EINVAL,
                                          "Target buffer too small for header");
                dst_header = reinterpret_cast<Virtio_net::Hdr *>(dst.pos);
                trace.printf("\tCopying header to %p (size: %u)\n",
                             dst.pos, dst.left);
                /*
                 * Header and csum offloading/general segmentation offloading
                 *
                 * We just copy the original header from source to
                 * destination and have to consider three different
                 * cases:
                 * - no flags are set
                 *   - we got a packet that is completely checksummed
                 *     and correctly fragmented, there is nothing to
                 *     do other then copying.
                 * - virtio_net_hdr_f_needs_csum set
                 *  - the packet is partially checksummed; if we would
                 *     send the packet out on the wire we would have
                 *     to calculate checksums now. But here we rely on
                 *     the ability of our guest to handle partially
                 *     checksummed packets and simply delegate the
                 *     checksum calculation to them.
                 * - gso_type != gso_none
                 *  - the packet needs to be segmented; if we would
                 *     send it out on the wire we would have to
                 *     segment it now. But again we rely on the
                 *     ability of our guest to handle gso
                 *
                 * We currently assume that our guests negotiated
                 * virtio_net_f_guest_*, this needs to be checked in
                 * the future.
                 *
                 * We also discussed the usage of
                 * virtio_net_hdr_f_data_valid to remove the need to
                 * checksum packets at all. But since our clients send
                 * partially checksummed packets anyway the only
                 * interesting case would be a packet without
                 * net_hdr_f_needs_checksum set. In that case we would
                 * signal that we checked the checksum and the
                 * checksum is actually correct. Since we do not know
                 * the origin of the packet (it could have been send
                 * by an external node and could have been routed to
                 * u) we can not signal this without actually
                 * verifying the checksum. Otherwise a packet with an
                 * invalid checksum could be successfully delivered.
                 */
                total = sizeof(Virtio_net::Hdr);
                src.copy_header(dst_header);
                mangle.rewrite_hdr(dst_header);
                dst.skip(total);
              }
            ++num_merged;
          }

        bool has_dst_buffer = !dst.done();
        if (!has_dst_buffer)
          try
            {
              // The current dst buffer is full, try to get next chained buffer.
              has_dst_buffer = dst_req_proc.next(dst_dev->mem_info(), &dst);
            }
          catch (L4virtio::Svr::Bad_descriptor &e)
            {
              Dbg(Dbg::Request, Dbg::Warn, "REQ")
                .printf("%s: bad descriptor exception: %s - %i"
                        " -- signal device error in destination device %p.\n",
                        __PRETTY_FUNCTION__, e.message(), e.error, dst_dev);
              dst_dev->device_error();
              return Result::Exception; // Must not touch the dst queues anymore.
            }

        if (has_dst_buffer)
          {
            auto &src_buf = src.cur_buf();
            trace.printf("\tCopying %p#%p:%u (%x) -> %p#%p:%u  (%x)\n",
                         src_port, src_buf.pos, src_buf.left, src_buf.left,
                         static_cast<Port_iface *>(this),
                         dst.pos, dst.left, dst.left);

            total += mangle.copy_pkt(dst, src_buf);
          }
        else if (negotiated_features().mrg_rxbuf())
          {
            // save descriptor information for later
            trace.printf("\tSaving descriptor for later\n");
            consumed.push_back(Consumed_entry(dst_head, total));
            total = 0;
            dst_head = L4virtio::Svr::Virtqueue::Head_desc();
          }
        else
          {
            trace.printf("\tTransfer failed, destination buffer too small, dropping.\n");
            // Abort incomplete transfer.
            dst_queue->rewind_avail(dst_head);
            return Result::Dropped;
          }
      }

    /*
     * Finalize the Request delivery. Call `finish()` on the destination
     * port's receive queue, which will result in triggering the destination
     * client IRQ.
     */

    if (!dst_header)
      {
        if (!total)
          trace.printf("\tTransfer - not started yet, dropping\n");
        return Result::Dropped;
      }

    if (consumed.empty())
      {
        assert(dst_head);
        assert(num_merged == 1);
        trace.printf("\tTransfer - Invoke dst_queue->finish()\n");
        dst_header->num_buffers = 1;
        dst_queue->finish(dst_head, dst_dev, total);
      }
    else
      {
        assert(dst_head);
        dst_header->num_buffers = num_merged;
        consumed.push_back(Consumed_entry(dst_head, total));
        trace.printf("\tTransfer - Invoke dst_queue->finish(iter)\n");
        dst_queue->finish(consumed.begin(), consumed.end(), dst_dev);
      }
    return Result::Delivered;
  }
};

/**\}*/
