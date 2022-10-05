#pragma once
#include <cmath>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <unordered_set>

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
int nthOccurrence(const std::string& str, const std::string& findMe, int nth);
// Adjust date string by a number of days +/-
std::string datePlusDays(const std::string& datestr, int days);

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

template<typename key_equal_t, typename hasher_t, bool order_conservative, typename container_t>
void remove_duplicates(container_t& container)
{
    using value_type = typename container_t::value_type;

    if constexpr(order_conservative)
    {
        std::unordered_set<value_type, hasher_t, key_equal_t> s;
        const auto predicate = [&s](const value_type& value){return !s.insert(value).second;};
        container.erase(std::remove_if(container.begin(), container.end(), predicate),
                                container.end());
    }
    else
    {
        const std::unordered_set<value_type, hasher_t, key_equal_t> s(container.begin(),
                                                                        container.end());
        container.assign(s.begin(), s.end());
    }
} 

}
