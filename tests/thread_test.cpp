#include <atomic>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

TEST(Threading, SimpleCounter)
{
    std::atomic<int> counter{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&counter] {
            for (int j = 0; j < 1000; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(counter.load(), 4000);
}
