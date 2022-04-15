//
// Created by Rory McStay on 06/07/2021.
//

#ifndef TRADINGO_SIMPLEMOVINGAVERAGE_H
#define TRADINGO_SIMPLEMOVINGAVERAGE_H

#include <stdint.h>
#include "Utils.h"

template <class input_t = long, class sum_t = long>
class SimpleMovingAverage {
    long count;
public:
    using input_type = input_t;
    using size_t = long;
    input_t operator()(input_t input);
    SimpleMovingAverage() = default;

    SimpleMovingAverage(long N_, double primedCount_)
    :   N(N_)
    ,   previousInputs(new input_t[N_]{})
    ,   primed(false)
    ,   primedCount(primedCount_*N_)
    ,   count(0){}

private:
    long N                 = 10;
    long primedCount       = 10;
    long index             = 0;
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
    if (++count == primedCount) {
        LOGINFO("SimpleMovingAverage is primed");
        primed = true;
    }
    return (sum) / N;
}


#endif //TRADINGO_SIMPLEMOVINGAVERAGE_H
