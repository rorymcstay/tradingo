//
// Created by Rory McStay on 19/06/2021.
//
#include "Params.h"
#include <set>
#include <string>

Params::Params(std::string str_)
:   _data()
,   _paramString(std::move(str_)) {
    parse_params_string(_paramString, *this);
}

std::string &Params::operator[](const std::string &key_) {
    return _data[key_];
}

std::string &Params::at(const std::string &key_) {
    try {
        return _data.at(key_);
    }
    catch (std::exception& ex) {
        std::stringstream error;
        error << "Missing mandatory parameter! " << LOG_VAR(key_) << LOG_VAR(_paramString);
        throw std::runtime_error(error.str());
    }
}

web::json::value Params::asJson() const {
    web::json::value retVal;
    for (auto& pair : _data) {
        auto key = pair.first;
        key[0] = tolower(key[0]);

        // TODO Mandatory params
        std::set<char> digit = {'-', '0','1','2','3', '4','5','6','7','8','9'};
        std::set<std::string> true_false = {"True", "False", "true", "false"};
        if (digit.contains(pair.second.front())
                and key.find("Timestamp")==std::string::npos
                and key.find("Time")==std::string::npos
                and key.find("timestamp")==std::string::npos
                and key.find("fundingInterval")==std::string::npos
                and key.find("sessionInterval")==std::string::npos
                and key.find("sessionInterval")==std::string::npos
                and key.find("closingTime")==std::string::npos
                and key.find("listing")==std::string::npos
                and key.find("front")==std::string::npos
        ) {
            // cast to double
            retVal[key] = web::json::value(std::atof(pair.second.c_str()));
            retVal[key].as_double();
        } else if (true_false.contains(pair.second)) {
            if (pair.second == "True") {
                retVal[key] = web::json::value(true);
            }
            if (pair.second == "False") {
                retVal[key] = web::json::value(false);
            }
        } else {
            // take it as string
            retVal[key] = web::json::value(pair.second);
        }
    }
    // add timestamp if it is missing
    if (_data.find("timestamp") == _data.end()) {
        retVal["timestamp"] = web::json::value(utility::datetime::utc_now().to_string());

    }
    return retVal;
}

const std::string &Params::at(const std::string &key_, const std::string &default_) {
    if (_data.find(key_) == _data.end()) {
        return default_;
    }
    return at(key_);
}
