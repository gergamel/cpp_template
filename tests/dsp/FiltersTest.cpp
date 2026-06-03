#include "dsp/Filters.hpp"
#include <gtest/gtest.h>

TEST(FirFilter, ReturnsCorrectLength)
{
    const std::vector<float> input = {1.0f, 2.0f, 3.0f, 4.0f};
    const std::vector<float> coeffs = {0.5f, 0.5f};
    const auto output = dsp::applyFIR(input, coeffs);

    ASSERT_EQ(output.size(), input.size());
    EXPECT_FLOAT_EQ(output[0], 0.5f);
    EXPECT_FLOAT_EQ(output[1], 1.5f);
    EXPECT_FLOAT_EQ(output[2], 2.5f);
    EXPECT_FLOAT_EQ(output[3], 3.5f);
}

TEST(MovingAverage, HandlesWindowLargerThanInput)
{
    const std::vector<float> input = {1.0f, 2.0f, 3.0f};
    const auto output = dsp::movingAverage(input, 5);

    EXPECT_FLOAT_EQ(output[0], 1.0f);
    EXPECT_FLOAT_EQ(output[1], 1.5f);
    EXPECT_FLOAT_EQ(output[2], 2.0f);
}
