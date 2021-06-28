#ifndef TRADINGO_TESTUTILS_H
#define TRADINGO_TESTUTILS_H
#define _TURN_OFF_PLATFORM_STRING
#include <cpprest/json.h>
#include "Event.h"

inline
std::string getEventTypeFromString(const std::string &marketDataString) {
    return marketDataString.substr(0, marketDataString.find(' '));
}


template<typename T>
std::shared_ptr<T> fromJson(web::json::value value_) {
    auto obj = std::make_shared<T>();
    obj->fromJson(value_);
    return obj;
}

#endif //TRADINGO_TESTUTILS_H
