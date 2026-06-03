#pragma once

#include <vector>
#include <cstddef>

namespace dsp {

std::vector<float> applyFIR(std::vector<float> const& input, std::vector<float> const& coefficients);
std::vector<float> movingAverage(std::vector<float> const& input, size_t windowSize);
std::vector<float> highpass(std::vector<float> const& input, float alpha);

} // namespace dsp
