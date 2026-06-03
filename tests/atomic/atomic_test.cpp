#include <atomic>
#include <thread>
#include "atomic/SpscQueue.hpp"
#include <gtest/gtest.h>

TEST(Atomic, SpscQueuePushPopAcrossThreads)
{
    atomic::SpscQueue<int, 128> queue;
    std::atomic<bool> done{false};
    std::thread producer([&] {
        for (int i = 0; i < 100; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
        done.store(true, std::memory_order_release);
    });

    int valueCount = 0;
    while (!done.load(std::memory_order_acquire) || !queue.empty()) {
        auto value = queue.pop();
        if (value) {
            ++valueCount;
        } else {
            std::this_thread::yield();
        }
    }

    producer.join();
    EXPECT_EQ(valueCount, 100);
}
