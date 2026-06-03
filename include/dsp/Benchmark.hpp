#pragma once

#include <chrono>
#include <vector>

namespace dsp {

struct BenchmarkResult {
    double megaSamplesPerSecond;
    std::chrono::nanoseconds elapsed;
};

BenchmarkResult runFirBenchmark(std::vector<float> const& input,
                                std::vector<float> const& coefficients,
                                size_t iterations = 100);

} // namespace dsp
