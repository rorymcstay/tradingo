#ifndef TRADINGO_EXPONENTIALMOVINGAVERAGE_H
#define TRADINGO_EXPONENTIALMOVINGAVERAGE_H

#include <stdint.h>
#include "Utils.h"


template <typename input_t, class sum_t>
class ExponentialMovingAverage {

public:
    using input_type = input_t;
    using size_t = long;
    input_t operator()(input_t input);

    ExponentialMovingAverage(input_t alpha_, long primed_count_=10.0)
    :   _alpha(alpha_)
    ,   _primed(false)
    ,   _primed_count(primed_count_)
    ,   _present_value(0.0)
    ,   _count(0) {
    }

private:
    long _primed_count;
    bool _primed;
    input_t _alpha;
    input_t _present_value;
    long _count;

    ExponentialMovingAverage& calculate(input_t in) {
        _present_value = in * (_alpha) + _present_value * (1.0f - _alpha);
        return *this;
    }

public:
    bool is_ready() const { return _primed; }

    ExponentialMovingAverage& set_alpha(input_t value) {
        _alpha = value;
        return *this;
    }

};


template<class input_t, class sum_t>
input_t ExponentialMovingAverage<input_t, sum_t>::operator()(input_t input) {
    if (++_count == _primed_count) {
        LOGINFO("ExponentialMovingAverage is primed");
        _primed = true;
    }
    calculate(input);
    return _present_value;
}


#endif //TRADINGO_EXPONENTIALMOVINGAVERAGE_H
