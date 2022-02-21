#pragma once
#include <cmath>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>

#include "aixlog.hpp"
#include <ModelBase.h>

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOGINFO(msg_)  LOG(INFO) << "\t(" << std::this_thread::get_id() << ")\t" << msg_ << " |" <<  __FILENAME__ << ":" << __LINE__ << '\n'
#define LOGDEBUG(msg_) LOG(DEBUG) << "\t(" << std::this_thread::get_id() << ")\t" << msg_ << " |" <<  __FILENAME__ << ":" << __LINE__ << '\n'
#define LOGWARN(msg_)  LOG(WARNING) << "\t(" << std::this_thread::get_id() << ")\t" << msg_ << " |" <<  __FILENAME__ <<  ":" << __LINE__ << '\n'
#define LOGERROR(msg_) LOG(ERROR) << "\t(" << std::this_thread::get_id() << ")\t" << AixLog::Color::red << msg_ << AixLog::Color::none << " |" <<  __FILENAME__ << ":" << __LINE__ << '\n'

#define LOG_VAR(var_) #var_ << "='" << var_ << "', "
#define LOG_NVP(name_, var_) name_ << "=" << var_ << " "

namespace tradingo_utils {

using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;

std::string formatTime(timestamp_t time_);
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template<typename T>
bool almost_equal(T num1, T num2, double tolerance=0.000001)
{
    return std::abs(num1 - num2) < tolerance;
}

template<typename T>
bool less_than(T num1, T num2)
{
    return !almost_equal(num1, num2) and num1 < num2;
}
template<typename T>
bool greater_than(T num1, T num2)
{
    return !almost_equal(num1, num2) and num1 > num2;
}

template<typename T>
bool greater_equal(T num1, T num2)
{
    return almost_equal(num1, num2) or num1 > num2;
}

template<typename T>
bool less_equal(T num1, T num2)
{
    return almost_equal(num1, num2) or num1 < num2;
}
using index_t = long;

long time_diff(utility::datetime time1_, utility::datetime time2_, const std::string& interval_="milliseconds");

}
