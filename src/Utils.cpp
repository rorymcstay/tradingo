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
