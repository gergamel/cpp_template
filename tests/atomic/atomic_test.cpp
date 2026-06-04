#include <gtest/gtest.h>
#include <fmt/core.h>
#include <gsl/span>
#include <stdalign.h>
#include <atomic>
#include <thread>
#include <chrono>

#if defined(__s390__) || defined(__s390x__)
    #define LOCKFREE_CACHELINE_BYTES 256
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
    #define LOCKFREE_CACHELINE_BYTES 128
#else
    #define LOCKFREE_CACHELINE_BYTES 64
#endif

template <std::size_t N>
struct lockfree_ring_base {
    static_assert((N & (N - 1)) == 0, "N must be power of two");
public:
    typedef std::size_t size_t;
    static constexpr const size_t max_idx = N;
protected:
    /******************************************************************************/
    std::atomic<size_t> wr_idx_;
    std::atomic<size_t> rd_idx_;
public:
    /******************************************************************************/
    lockfree_ring_base() noexcept : wr_idx_(0),rd_idx_(0) {}
    /******************************************************************************/   
    bool is_lock_free(void) const noexcept {
        return wr_idx_.is_lock_free() && rd_idx_.is_lock_free();
    }
    /******************************************************************************/   
    static constexpr size_t next_index(size_t arg) noexcept {
        return (arg + 1) & (max_idx - 1);
    }
    // occupancy
    static constexpr size_t read_available(size_t wr, size_t rd) noexcept {
        const size_t ret = (wr + max_idx - rd) & (max_idx - 1);
        return ret;
    }
    static constexpr size_t write_available(size_t wr, size_t rd) noexcept {
        const size_t ret = (rd + max_idx - wr - 1) & (max_idx - 1);
        return ret;
    }
    /******************************************************************************/ 
    size_t read_available() const noexcept {
        size_t       wr = wr_idx_.load(std::memory_order_acquire);
        const size_t rd  = rd_idx_.load(std::memory_order_relaxed);
        return read_available(wr,rd);
    }
    size_t write_available() const noexcept {
        size_t       wr = wr_idx_.load(std::memory_order_relaxed);
        const size_t rd = rd_idx_.load(std::memory_order_acquire);
        return write_available(wr,rd);
    }
    /******************************************************************************/
    size_t rd_idx() const noexcept {
        const size_t rd = rd_idx_.load(std::memory_order_relaxed);
        return rd;
    }
    size_t wr_idx() const noexcept {
        const size_t wr = wr_idx_.load(std::memory_order_relaxed);
        return wr;
    }
    /******************************************************************************/
    size_t pop() noexcept {
        const size_t cur_rd = rd_idx_.load(std::memory_order_acquire);
        const size_t cur_wr = wr_idx_.load(std::memory_order_relaxed);
        const size_t avail = read_available(cur_wr,cur_rd);
        if (avail == 0) return 0;
        size_t nxt_rd = next_index(cur_rd);
        rd_idx_.store(nxt_rd,std::memory_order_release);
        return 1;
    }
    size_t push() noexcept {
        const size_t cur_wr = wr_idx_.load(std::memory_order_acquire);
        const size_t cur_rd = rd_idx_.load(std::memory_order_relaxed);
        const size_t avail  = write_available(cur_wr,cur_rd);
        if (avail == 0) return 0;
        size_t nxt_wr = next_index(cur_wr);
        wr_idx_.store(nxt_wr,std::memory_order_release);
        return 1;
    }
};

template<typename Ts,std::size_t BLKSZ,std::size_t NBLKS>
struct SPSCRing : lockfree_ring_base<NBLKS> {
public:
    typedef lockfree_ring_base<NBLKS> base_t;
    typedef std::array<Ts,BLKSZ>      blk_t;
    typedef std::array<blk_t,NBLKS>   mem_t;
    typedef gsl::span<Ts,BLKSZ>       span_t;
    /******************************************************************************/
    constexpr std::optional<span_t> cur_wr() {
        if (this->write_available()<1) return std::nullopt;

        size_t wr = this->wr_idx();
        span_t result{data_[wr]};
        return result;
    }
    constexpr std::optional<span_t> cur_rd() {
        if (this->read_available()<1) return std::nullopt;

        size_t rd = this->rd_idx();
        span_t result{data_[rd]};
        return result;
    }
protected:
    mem_t data_;
};

TEST(lockfree_ring_base, basic_directed) {
    static constexpr const size_t RING_SZ = 8;
    static constexpr const size_t N = 4;
    using ring_t = lockfree_ring_base<RING_SZ>;
    ring_t ring;

    size_t wr = ring.wr_idx();
    for (size_t n=0;n<N;n++) {
        const size_t exp_wr = n % RING_SZ;
        EXPECT_EQ(wr,exp_wr);
        while (ring.push()==0) {
        }
        wr = ring.wr_idx();
    }

    size_t rd;
    for (size_t n=0;n<N;n++) {
        while (ring.pop()==0) {
        }
        rd = ring.rd_idx();
        const size_t exp_rd = (n+1) % RING_SZ;
        EXPECT_EQ(rd,exp_rd);
        rd = ring.rd_idx();
    }
}

TEST(lockfree_ring_base, lockfree_ring_base) {
    static constexpr const size_t RING_SZ = 8;
    static constexpr const size_t N = 100000;
    using ring_t = lockfree_ring_base<RING_SZ>;
    ring_t ring;

    EXPECT_TRUE(ring.is_lock_free());
    std::atomic<bool> done(false);
    size_t n_pushed   = 0;
    size_t n_popped   = 0;
    size_t push_fails = 0;
    size_t pop_fails  = 0;

    std::thread producer_thread([&] () {
        size_t wr = ring.wr_idx();
        for (size_t n=0;n<N;n++) {
            const size_t exp_wr = n % RING_SZ;
            EXPECT_EQ(wr,exp_wr);
            while (ring.push()==0) {
                push_fails++;
            }
            n_pushed++;
            wr = ring.wr_idx();
        }
        done = true;
    });
    std::thread consumer_thread([&] () {
        size_t rd;
        for (size_t n=1;n<=N;n++) {
            while (ring.pop()==0) {
                pop_fails++;
            }
            rd = ring.rd_idx();
            const size_t exp_rd = n % RING_SZ;
            n_popped++;
            EXPECT_EQ(rd,exp_rd);
            rd = ring.rd_idx();
        }
    });
    producer_thread.join();
    consumer_thread.join();
    ASSERT_EQ(n_pushed,n_popped);
    fmt::println("Pushed {} ({} fails)",n_pushed,push_fails);
    fmt::println("Popped {} ({} fails)",n_popped,pop_fails);
}

TEST(spsc_ring,concurrent_write_read) {
    using clk = std::chrono::steady_clock;

    /* This test covers (and acts as a reference example) for using 
     * boost::lockfree::spsc_queue as a fast ring buffer for fixed
     * block sizes, as we use with libiio for IQ radios.
     * 
     * Paramters
     * - N_BLK = Processing block size (e.g. IIO buffer size)
     * - N_CAP = Total queue capacity (should be an integer number of N_BLK)
     * - N_TOT = total samples to push through the ring during the tests
     */
    using sample_t = float;
    static constexpr const size_t BLKSZ   = 65536;          // Samples per block
    static constexpr const size_t N_BLKS  = 32;             // Block in the ring
    static constexpr const size_t N_LOOPS = N_BLKS*4;       // Total blocks to push through the ring
    static constexpr const size_t N_TOT   = N_LOOPS*BLKSZ;  // Total expected samples to process
    using ring_t = SPSCRing<sample_t,BLKSZ,N_BLKS>;
    static ring_t ring;

    std::atomic<bool> done(false);
    size_t wr_samples = 0;
    size_t rd_samples = 0;
    size_t wait_wr    = 0;
    size_t n_pushed   = 0;
    size_t n_popped   = 0;
    size_t push_fails = 0;
    size_t wait_rd    = 0;
    size_t pop_fails  = 0;
    EXPECT_TRUE(ring.is_lock_free());

    auto t0 = clk::now();
    std::thread producer_thread([&] () {
        std::optional<ring_t::span_t> cur_wr;
        for (size_t n=0;n<N_LOOPS;n++) {
            cur_wr = ring.cur_wr();
            while (!cur_wr.has_value()) {
                wait_wr++;
                cur_wr = ring.cur_wr();
            };
            EXPECT_TRUE(cur_wr.has_value());
            auto& sp = cur_wr.value();
            EXPECT_NE(sp.data(),nullptr);
            EXPECT_EQ(sp.size(),BLKSZ);

            // Pretend to write
            for (auto& x : sp) {
                x = 1e-6f*((float)(wr_samples++));
            }

            while (ring.push()==0) {
                ++push_fails;
            };
            n_pushed++;
        }
        done = true;
    });
    std::thread consumer_thread([&] () {
        std::optional<ring_t::span_t> cur_rd;
        for (size_t n=0;n<N_LOOPS;n++) {
            cur_rd = ring.cur_rd();
            while (!cur_rd.has_value()) {
                wait_rd++;
                cur_rd = ring.cur_rd();
            }
            EXPECT_TRUE(cur_rd.has_value());
            auto& sp = cur_rd.value();
            EXPECT_NE(sp.data(),nullptr);
            EXPECT_EQ(sp.size(),BLKSZ);

            // Pretend to read
            for (auto& x : sp) {
                const float exp_x = 1e-6f*((float)(rd_samples++));
                EXPECT_EQ(x,exp_x);
            }

            while (ring.pop()==0) {
                ++pop_fails;
            };
            n_popped++;
        }
    });
    producer_thread.join();
    consumer_thread.join();
    auto t1 = clk::now();
    double elapsed_secs  = duration_cast<std::chrono::duration<double,std::ratio<1UL,1UL>>>(t1 - t0).count();
    double msmps_per_sec = 1e-6*static_cast<double>(N_TOT)/elapsed_secs;

    ASSERT_EQ(rd_samples,N_TOT);
    ASSERT_EQ(wr_samples,N_TOT);
    ASSERT_EQ(n_pushed,N_LOOPS);
    ASSERT_EQ(n_popped,N_LOOPS);
    fmt::println("Pushed {} samples over {} blocks ({} waits, {} fails)",wr_samples,n_pushed,wait_wr,push_fails);
    fmt::println("Popped {} samples over {} blocks ({} waits, {} fails)",rd_samples,n_popped,wait_rd,pop_fails);
    fmt::println("{:.03f} Msps over {:.03f} seconds",msmps_per_sec,elapsed_secs);
}