#include "dsp/Filters.hpp"
#include <algorithm>

namespace dsp {

std::vector<float> applyFIR(std::vector<float> const& input, std::vector<float> const& coefficients)
{
    const size_t n = input.size();
    const size_t k = coefficients.size();
    std::vector<float> output(n);

    for (size_t i = 0; i < n; ++i) {
        float sum = 0.0f;
        for (size_t j = 0; j < k; ++j) {
            if (i >= j) {
                sum += input[i - j] * coefficients[j];
            }
        }
        output[i] = sum;
    }

    return output;
}

std::vector<float> movingAverage(std::vector<float> const& input, size_t windowSize)
{
    const size_t n = input.size();
    std::vector<float> output(n);
    float accumulator = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        accumulator += input[i];
        if (i >= windowSize) {
            accumulator -= input[i - windowSize];
        }
        output[i] = accumulator / static_cast<float>(std::min(i + 1, windowSize));
    }

    return output;
}

std::vector<float> highpass(std::vector<float> const& input, float alpha)
{
    const size_t n = input.size();
    std::vector<float> output(n);
    float previous = input.empty() ? 0.0f : input[0];

    for (size_t i = 0; i < n; ++i) {
        const float current = input[i];
        output[i] = alpha * (previous + current - (i > 0 ? input[i - 1] : 0.0f));
        previous = current;
    }

    return output;
}

} // namespace dsp
