#pragma once
#include <cstddef>
#include <atomic>
#include <array>
#include <atomic>
#include <thread>
#include <gsl/span_ext>
#include "avi/chrono.h"
#include "avi/io/types.h"

using std::size_t;

/********************************************************************************
 * Ring Trace Structures
 *******************************************************************************/
enum RingEventType : size_t {
    Invalid          = 0,
    WriteAcquireDrop = 1,
    WriteAcquireFail = 2,
    WriteAcquireOk   = 3,
    WritePublish     = 4,
    ReadAcquireEmpty = 5,
    ReadAcquireOk    = 6,
    ReadRelease      = 7,
};

static constexpr const char* RingEventString[] = {
    "WrAcqDrop ",
    "WrAcqFail ",
    "WrAcqOk   ",
    "WrPublish ",
    "RdAcqEmpty",
    "RdAcqOk   ",
    "RdRelease ",
};


struct RingEvent {
    using clk        = std::chrono::steady_clock;
    using time_point = clk::time_point;

    time_point      tp;
    size_t          blk_id;
    RingEventType   event;
    constexpr RingEvent() noexcept :
        blk_id(0),
        event(RingEventType::Invalid)
    {}
};

template <size_t Nev>
struct RingEventTrace {
    using clk        = typename RingEvent::clk;
    using time_point = typename RingEvent::time_point;

    RingEvent   ev[Nev];
    size_t      idx;
    time_point  min_ts;

    constexpr RingEventTrace() :
        idx(0),
        min_ts(std::numeric_limits<RingEvent::time_point>::max())
    {}

    void append(size_t blk_id, RingEventType event) noexcept {
        auto ts = clk::now();//__builtin_ia32_rdtsc();
        if (ts<min_ts)  min_ts = ts;
        ev[idx].ts     = ts;
        ev[idx].blk_id = blk_id;
        ev[idx].event  = event;
        idx = (idx==(Nev-1)) ? 0 : idx+1;
    }
    void print_trace(time_point ofs, size_t limit, double secs_per_tick) const {
        size_t n = idx;
        size_t l = 0;
        for (;;) {
            double ts = static_cast<double>(ev[n].ts-ofs)*secs_per_tick;
            printf("[%u] %.06f %u %s\n", n, ts, ev[n].blk_id, RingEventString[ev[n].event]);
            if (n==0) break;
            n -= 1;
            l++;
            if (l>=limit) break;
        }
    }
};

#if defined(__s390__) || defined(__s390x__)
    #define LOCKFREE_CACHELINE_BYTES 256
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
    #define LOCKFREE_CACHELINE_BYTES 128
#else
    #define LOCKFREE_CACHELINE_BYTES 64
#endif

// ---------------------------
// Wait-free SPSC ring (drop-oldest on full by default)
// ---------------------------
// The ring stores Blocks directly (no indirection), all pre-allocated.
template<typename Ts,size_t BLKSZ,size_t NBLKS>
struct SPSCRing {
public:
    using BlockType = std::array<Ts,BLKSZ>;
    using ReadSpan = gsl::span<Ts,BLKSZ>;

    // Write spans must be dynamic extent to allow resizing during write
    using WriteSpan = gsl::span<Ts,gsl::dynamic_extent>;
    static constexpr const size_t buflen = BLKSZ;

protected:
    static_assert((NBLKS & (NBLKS - 1)) == 0, "N must be power of two");
    static const int padding_size = LOCKFREE_CACHELINE_BYTES - sizeof(size_t);
    std::atomic<size_t>    write_idx; // next index producer will write
    char padding1[padding_size]; /* force read_index and write_index to different cache lines */
    std::atomic<size_t>    read_idx;  // next index consumer will read
    char padding2[padding_size]; /* force read_index and write_index to different cache lines */
    std::array<BlockType,NBLKS> blocks;
    RingEventTrace<1024>        wr_trace;
    RingEventTrace<1024>        rd_trace;

public:
    static constexpr size_t next_idx(size_t arg) {
        return (arg + 1) & (NBLKS - 1);
    }

    [[nodiscard]] auto is_lock_free() const->bool {
        return write_idx.is_lock_free() && read_idx.is_lock_free();
    }

    [[nodiscard]] constexpr auto trace_min_tick() const noexcept->time_point {
        return std::min(wr_trace.min_ts, rd_trace.min_ts);
    }
    void print_trace(time_point tick_ofs, size_t limit, double secs_per_tick) const noexcept {
        printf("=====================================\n");
        printf("Write Events\n");
        wr_trace.print_trace(tick_ofs,limit,secs_per_tick);
        printf("=====================================\n");
        printf("=====================================\n");
        printf("Read Events\n");
        rd_trace.print_trace(tick_ofs,limit,secs_per_tick);
        printf("=====================================\n");
    }

    // Try to acquire a Block pointer for writing.
    // If ring is full, either return nullptr (non-blocking fail) OR drop oldest and return a block.
    // Here we provide two variants; default is drop-oldest.
    [[nodiscard]] constexpr auto try_acquire_write_drop_oldest() noexcept->WriteSpan {
        const size_t w = write_idx.load(std::memory_order_relaxed);
        const size_t r = read_idx.load(std::memory_order_acquire);
        if (next_idx(w) == r) {
            size_t n = next_idx(r);
            // FULL -> advance read_idx to drop the oldest
            read_idx.store(n, std::memory_order_release);
            wr_trace.append(n,RingEventType::WriteAcquireDrop);
            // now there is space
        }
        wr_trace.append(w,RingEventType::WriteAcquireOk);
        // return pointer to block to write
        return WriteSpan{blocks[w].data(),blocks[w].size()};
    }

    // Try to acquire a write block but return nullptr if full (no overwrite)
    [[nodiscard]] constexpr auto try_acquire_write_no_overwrite() noexcept->std::optional<WriteSpan> {
        const size_t w = write_idx.load(std::memory_order_relaxed);
        const size_t r = read_idx.load(std::memory_order_acquire);
        if (next_idx(w) == r) {
            wr_trace.append(w,RingEventType::WriteAcquireFail);
            return std::nullopt; // full
        }
        wr_trace.append(w,RingEventType::WriteAcquireOk);
        return WriteSpan{blocks[w].data(),blocks[w].size()};
    }

    [[nodiscard]] constexpr auto wait_acquire_write(std::atomic<bool>& run_flag) noexcept->WriteSpan {
        std::optional<WriteSpan> b = try_acquire_write_no_overwrite();
        while (!b.has_value() && run_flag.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
            b = try_acquire_write_no_overwrite();
        }
        if (b.has_value()) {
            return b.value();
        }
        return {};
    }

    // Publish the write (advance write index)
    constexpr void publish_write() noexcept {
        const size_t w = write_idx.load(std::memory_order_relaxed);
        write_idx.store(next_idx(w), std::memory_order_release);
        wr_trace.append(next_idx(w),RingEventType::WritePublish);
    }

    // Try to acquire a block for reading; return nullptr if empty
    [[nodiscard]] constexpr auto try_acquire_read() noexcept->std::optional<ReadSpan> {
        const size_t r = read_idx.load(std::memory_order_relaxed);
        const size_t w = write_idx.load(std::memory_order_acquire);
        if (r == w) {
            // rd_trace.append(0,RingEventType::ReadAcquireEmpty);
            return std::nullopt; // full
        } 
        rd_trace.append(0,RingEventType::ReadAcquireOk);
        return ReadSpan{blocks[r & (NBLKS - 1)]};
    }

    // Release after read (advance read index)
    constexpr void release_read() noexcept {
        const size_t r = read_idx.load(std::memory_order_relaxed);
        read_idx.store(next_idx(r),std::memory_order_release);
        rd_trace.append(next_idx(r),RingEventType::ReadRelease);
    }

    // Info helpers
    [[nodiscard]] constexpr auto occupancy() const noexcept->size_t {
        const size_t w = write_idx.load(std::memory_order_acquire);
        const size_t r = read_idx.load(std::memory_order_acquire);
        return (w + NBLKS - r) & (NBLKS - 1);
    }
};
// static_assert(IoRing<SPSCRing<complex_i64,80000,8>>);
