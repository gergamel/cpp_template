#include <iostream>
#include "dsp/Benchmark.hpp"

int main()
{
    const size_t sampleCount = 1 << 18; // 262k samples
    std::vector<float> input(sampleCount, 1.0f);
    std::vector<float> coefficients = {0.1f, 0.15f, 0.5f, 0.15f, 0.1f};

    const dsp::BenchmarkResult result = dsp::runFirBenchmark(input, coefficients, 50);
    std::cout << "FIR benchmark: " << result.megaSamplesPerSecond << " MS/s" << std::endl;
    std::cout << "Elapsed: " << (static_cast<double>(result.elapsed.count()) / 1e9) << " s" << std::endl;
    return 0;
}
