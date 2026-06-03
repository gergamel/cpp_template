#include "dsp/Benchmark.hpp"
#include <gtest/gtest.h>

TEST(Benchmark, ReportsPositiveThroughput)
{
    std::vector<float> input(1 << 16, 1.0f);
    std::vector<float> coeffs = {0.25f, 0.5f, 0.25f};

    const dsp::BenchmarkResult result = dsp::runFirBenchmark(input, coeffs, 10);
    EXPECT_GT(result.megaSamplesPerSecond, 0.0);
}
