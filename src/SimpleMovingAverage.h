//
// Created by Rory McStay on 06/07/2021.
//

#ifndef TRADINGO_SIMPLEMOVINGAVERAGE_H
#define TRADINGO_SIMPLEMOVINGAVERAGE_H

#include <stdint.h>

template <uint8_t N, class input_t = uint16_t, class sum_t = uint32_t>
class SimpleMovingAverage {
public:
    input_t operator()(input_t input);

    static_assert(
            sum_t(0) < sum_t(-1),  // Check that `sum_t` is an unsigned type
            "Error: sum data type should be an unsigned integer, otherwise, "
            "the rounding operation in the return statement is invalid.");

private:
    uint8_t index             = 0;
    input_t previousInputs[N] = {};
    sum_t sum                 = 0;
};

template<uint8_t N, class input_t, class sum_t>
input_t SimpleMovingAverage<N, input_t, sum_t>::operator()(input_t input) {
    sum -= previousInputs[index];
    sum += input;
    previousInputs[index] = input;
    if (++index == N)
        index = 0;
    return (sum + (N / 2)) / N;
}


#endif //TRADINGO_SIMPLEMOVINGAVERAGE_H
