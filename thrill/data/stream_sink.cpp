/*******************************************************************************
 * thrill/data/stream_sink.cpp
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <thrill/data/stream_sink.hpp>

namespace thrill {
namespace data {

StreamSink::StreamSink(BlockPool& block_pool,
                       net::DispatcherThread* dispatcher,
                       net::Connection* connection,
                       MagicByte magic,
                       StreamId stream_id, size_t host_rank,
                       size_t my_local_worker_id,
                       size_t peer_rank,
                       size_t peer_local_worker_id)
    : BlockSink(block_pool, my_local_worker_id),
      dispatcher_(dispatcher),
      connection_(connection),
      magic_(magic),
      id_(stream_id),
      host_rank_(host_rank),
      my_local_worker_id_(my_local_worker_id),
      peer_rank_(peer_rank),
      peer_local_worker_id_(peer_local_worker_id) {
    logger()
        << "class" << "StreamSink"
        << "event" << "open"
        << "stream" << id_
        << "src_worker" << (host_rank_ * workers_per_host()) + my_local_worker_id_
        << "peer_host" << peer_rank_
        << "tgt_worker" << (peer_rank_ * workers_per_host()) + peer_local_worker_id_;
}

void StreamSink::AppendBlock(const PinnedBlock& block) {
    if (block.size() == 0) return;

    sLOG << "StreamSink::AppendBlock" << block;

    StreamBlockHeader header(magic_, block);
    header.stream_id = id_;
    header.sender_rank = host_rank_;
    header.sender_local_worker_id = my_local_worker_id_;
    header.receiver_local_worker_id = peer_local_worker_id_;

    if (debug) {
        sLOG << "sending block" << common::Hexdump(block.ToString());
    }

    net::BufferBuilder bb;
    header.Serialize(bb);

    net::Buffer buffer = bb.ToBuffer();
    assert(buffer.size() == BlockHeader::total_size);

    byte_counter_ += buffer.size() + block.size();
    ++block_counter_;

    dispatcher_->AsyncWrite(
        *connection_,
        // send out Buffer and Block, guaranteed to be successive
        std::move(buffer), block);
}

void StreamSink::Close() {
    assert(!closed_);
    closed_ = true;

    sLOG << "sending 'close stream' from host_rank" << host_rank_
         << "worker" << my_local_worker_id_
         << "to" << peer_rank_
         << "worker" << peer_local_worker_id_
         << "stream" << id_;

    StreamBlockHeader header;
    header.magic = magic_;
    header.stream_id = id_;
    header.sender_rank = host_rank_;
    header.sender_local_worker_id = my_local_worker_id_;
    header.receiver_local_worker_id = peer_local_worker_id_;

    net::BufferBuilder bb;
    header.Serialize(bb);

    net::Buffer buffer = bb.ToBuffer();
    assert(buffer.size() == BlockHeader::total_size);

    byte_counter_ += buffer.size();
    ++block_counter_;

    dispatcher_->AsyncWrite(*connection_, std::move(buffer));

    logger()
        << "class" << "StreamSink"
        << "event" << "close"
        << "stream" << id_
        << "src_worker" << (host_rank_ * workers_per_host()) + my_local_worker_id_
        << "peer_host" << peer_rank_
        << "tgt_worker" << (peer_rank_ * workers_per_host()) + peer_local_worker_id_
        << "bytes" << byte_counter_
        << "blocks" << block_counter_
        << "timespan" << tx_timespan_;
}

} // namespace data
} // namespace thrill

/******************************************************************************/
