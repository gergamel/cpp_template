#include <iostream>
#include "dsp/Filters.hpp"

int main()
{
    constexpr size_t length = 16;
    std::vector<float> input(length, 1.0f);
    const auto output = dsp::applyFIR(input, {0.25f, 0.5f, 0.25f});

    std::cout << "DSP chain sample output:";
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << (i ? ", " : " ") << output[i];
    }
    std::cout << '\n';
    return 0;
}
