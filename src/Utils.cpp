//
// Created by Rory McStay on 07/07/2021.
//
#include "Utils.h"


std::string formatTime(timestamp_t time_) {
    auto outTime = std::chrono::system_clock::to_time_t(time_);
    std::stringstream ss;
    ss << std::put_time(localtime(&outTime), "%Y-%m-%d");
    return ss.str();
}

long time_diff(utility::datetime time1_, utility::datetime time2_, const std::string& interval_) {
    if (not (time1_.is_initialized() and time2_.is_initialized())) {
        throw std::runtime_error("Times are not initialised! ");
    }
    auto diff = time1_.to_interval() - time2_.to_interval();
    if (interval_ == "milliseconds") {
        return diff / 10000;
    } else if (interval_ == "microseconds") {
        return diff;
    } else if (interval_ == "seconds") {
        return diff / 100000;
    } else {
        LOGERROR("interval not recognised!" << LOG_VAR(interval_));
        return diff;
    }
}
