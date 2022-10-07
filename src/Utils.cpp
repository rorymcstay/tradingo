//
// Created by Rory McStay on 07/07/2021.
//
#include "Utils.h"


std::string tradingo_utils::formatTime(tradingo_utils::timestamp_t time_) {
    auto outTime = std::chrono::system_clock::to_time_t(time_);
    std::stringstream ss;
    ss << std::put_time(localtime(&outTime), "%Y-%m-%d");
    return ss.str();
}


long tradingo_utils::time_diff(utility::datetime time1_, utility::datetime time2_, const std::string& interval_) {
    if (not (time1_.is_initialized() and time2_.is_initialized())) {
        throw std::runtime_error("Times are not initialised! ");
    }
    long diff;
    if (time2_ > time1_) {
        diff =  -(time2_.to_interval() - time1_.to_interval());
    } else {
        diff = time1_.to_interval() - time2_.to_interval();
    }
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


int tradingo_utils::nthOccurrence(const std::string& str, const std::string& findMe, int nth)
{
    size_t  pos = 0;
    int     cnt = 0;

    while( cnt != nth )
    {
        pos+=1;
        pos = str.find(findMe, pos);
        if ( pos == std::string::npos )
            return -1;
        cnt++;
    }
    return pos;
}
// Adjust date by a number of days +/-
std::string tradingo_utils::datePlusDays(const std::string& datestr, int days)
{

    size_t pos = 0;
    std::string token;
    auto year = std::stoi(datestr.substr(0, nthOccurrence(datestr, "-", 1)));
    auto month = std::stoi(datestr.substr(nthOccurrence(datestr, "-", 1)+1, nthOccurrence(datestr, "-", 2)));
    auto day = std::stoi(datestr.substr(nthOccurrence(datestr, "-", 2)+1, datestr.size()));
    const time_t ONE_DAY = 24 * 60 * 60 ;

    // Seconds since start of epoch
    struct tm date = { 0, 0, 12 } ;  // nominal time midday (arbitrary).
    date.tm_year = year - 1900;
    date.tm_mon = month -1;
    date.tm_mday = day;
    time_t date_seconds = std::mktime( &date ) + (days * ONE_DAY) ;

    // Update caller's date
    // Use localtime because mktime converts to UTC so may change date
    date = *localtime( &date_seconds ) ; ;
    std::stringstream datestr_out;
    datestr_out 
        << date.tm_year + 1900
        << "-" << std::setfill('0') << std::setw(2) <<  date.tm_mon + 1
        << "-" << std::setfill('0') << std::setw(2) <<  date.tm_mday;
    return datestr_out.str();
}
