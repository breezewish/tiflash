// Copyright 2022 PingCAP, Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <Common/Exception.h>
#include <Core/Block.h>
#include <Storages/DeltaMerge/ReadThread/WorkQueue.h>
#include <Storages/DeltaMerge/SegmentReadResultChannel_fwd.h>

#include <boost/noncopyable.hpp>

namespace DB::DM
{

/**
 * A channel of results to be read from the ResultChannelInputStream.
 * There could be multiple producers generating the result, and
 * multiple consumers (ResultChannelInputStream) consuming the result.
 */
class SegmentReadResultChannel
    : private boost::noncopyable
    , public std::enable_shared_from_this<SegmentReadResultChannel>
{
public:
    class BlockStat
    {
    public:
        BlockStat()
            : pending_count(0)
            , pending_bytes(0)
            , total_count(0)
            , total_bytes(0)
        {}

        void push(const Block & blk)
        {
            pending_count.fetch_add(1, std::memory_order_relaxed);
            total_count.fetch_add(1, std::memory_order_relaxed);

            auto b = blk.bytes();
            pending_bytes.fetch_add(b, std::memory_order_relaxed);
            total_bytes.fetch_add(b, std::memory_order_relaxed);
        }

        void pop(const Block & blk)
        {
            if (likely(blk))
            {
                pending_count.fetch_sub(1, std::memory_order_relaxed);
                pending_bytes.fetch_sub(blk.bytes(), std::memory_order_relaxed);
            }
        }

        int64_t pendingCount() const
        {
            return pending_count.load(std::memory_order_relaxed);
        }

        int64_t pendingBytes() const
        {
            return pending_bytes.load(std::memory_order_relaxed);
        }

        int64_t totalCount() const
        {
            return total_count.load(std::memory_order_relaxed);
        }
        int64_t totalBytes() const
        {
            return total_bytes.load(std::memory_order_relaxed);
        }

    private:
        /// How many blocks are there in the channel.
        std::atomic<int64_t> pending_count;
        std::atomic<int64_t> pending_bytes;

        /// How many blocks were pushed to the channel.
        std::atomic<int64_t> total_count;
        std::atomic<int64_t> total_bytes;
    };

    struct Options
    {
        /// The number of "sources". It should be paired when calling `finish()`.
        /// Conventionally, `source` is `{store_id}_{segment_id}`.
        const UInt64 expected_sources;

        /// The structure of the block content. To avoid mistakes, we requires all
        /// blocks to follow the same structure.
        const Block header;

        const String & debug_tag;

        /// If we reaches this limit, we will consider this channel as full.
        /// This is not a hard limit.
        const UInt64 max_pending_blocks;

        const std::function<void(SegmentReadResultChannelPtr)> on_first_read;
    };


    explicit SegmentReadResultChannel(
        const Options & options);

    ~SegmentReadResultChannel();

    static std::shared_ptr<SegmentReadResultChannel> create(
        const Options & options)
    {
        return std::make_shared<SegmentReadResultChannel>(options);
    }

    /// Thread-safe. Called by produers.
    void pushBlock(Block && block);

    /// Thread-safe. Called by produers.
    /// Mark one "source" as finished.
    /// When all "sources" are finished, this channel is finished.
    /// Conventionally, `source` is `{store_id}_{segment_id}`.
    void finish(String debug_source_tag);

    /// Thread-safe. Called by produers.
    /// Immediately finish this channel and record the exception.
    void finishWithError(const DB::Exception & e);

    /// Thread-safe. Called by consumers.
    void popBlock(Block & block);

    /// Thread-safe. Called by consumers.
    bool tryPopBlock(Block & block);

    /// Thread-safe.
    UInt64 refConsumer();

    /// Thread-safe.
    UInt64 derefConsumer();

    /// Thread-safe.
    bool hasAliveConsumers() const;

    /// Thread-safe.
    bool valid() const;

    /// Thread-safe.
    /// Returns true if reaching max_pending_blocks.
    /// Note that this is a soft limit. You can still push more data to the queue.
    bool isFull() const;

    /// Thread-safe.
    /// TODO: Move to private. Currently PipelineOperators calls this externally.
    void triggerFirstRead();

public:
    const UInt64 expected_sources;
    const String debug_tag;
    const UInt64 max_pending_blocks;
    const Block header;

private:
    const std::function<void(SegmentReadResultChannelPtr)> on_first_read;
    const LoggerPtr log;

    mutable std::mutex mu;
    std::set<String> finished_sources;

    std::atomic<bool> is_finished = false;
    std::atomic<bool> has_error = false;
    DB::Exception exception;

    std::atomic<Int64> alive_consumers = 0;

    std::once_flag has_read_once;

private:
    WorkQueue<Block> q;

    /// Statistics about the current channel.
    BlockStat blk_stat;

    /// Statistics for all channels.
    inline static BlockStat global_blk_stat;
};


} // namespace DB::DM