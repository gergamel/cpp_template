#include "dsp/Benchmark.hpp"
#include "dsp/Filters.hpp"

namespace dsp {

BenchmarkResult runFirBenchmark(std::vector<float> const& input,
                                std::vector<float> const& coefficients,
                                size_t iterations)
{
    const size_t sampleCount = input.size();
    auto start = std::chrono::steady_clock::now();

    for (size_t iter = 0; iter < iterations; ++iter) {
        volatile auto output = applyFIR(input, coefficients);
        (void)output;
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const double samples = static_cast<double>(sampleCount) * static_cast<double>(iterations);
    const double elapsedSeconds = static_cast<double>(elapsed.count()) / 1.0e9;
    const double megaSamplesPerSecond = (samples / 1.0e6) / elapsedSeconds;

    return BenchmarkResult{megaSamplesPerSecond, elapsed};
}

} // namespace dsp
