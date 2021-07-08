//
// Created by Rory McStay on 06/07/2021.
//

#ifndef TRADINGO_SIMPLEMOVINGAVERAGE_H
#define TRADINGO_SIMPLEMOVINGAVERAGE_H

#include <stdint.h>

template <class input_t = uint16_t, class sum_t = uint32_t>
class SimpleMovingAverage {
public:
    input_t operator()(input_t input);
    SimpleMovingAverage() = default;

    SimpleMovingAverage(uint8_t N_, uint8_t primedCount_)
    :   N(N_)
    ,   previousInputs(new input_t[N_]{})
    ,   primed(false)
    ,   primedCount(primedCount_){}

private:
    uint8_t N                 = 10;
    uint8_t primedCount       = 10;
    uint8_t index             = 0;
    input_t * previousInputs;
    sum_t sum                 = 0;
    bool primed;
public:
    bool is_ready() const { return primed; }
};

template<class input_t, class sum_t>
input_t SimpleMovingAverage<input_t, sum_t>::operator()(input_t input) {
    sum -= previousInputs[index];
    sum += input;
    previousInputs[index] = input;
    if (++index == N) {
        index = 0;
    }
    if (index >= primedCount) {
        primed = true;
    }
    return (sum) / N;
}


#endif //TRADINGO_SIMPLEMOVINGAVERAGE_H
