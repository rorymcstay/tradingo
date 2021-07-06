//
// Created by Rory McStay on 19/06/2021.
//



#include "Params.h"

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
        // number like
        if (pair.first.find("Size") != std::string::npos
            or pair.first.find("Qty") != std::string::npos
            or pair.first.find("Price") != std::string::npos
            or pair.first.find("price") != std::string::npos
            or pair.first.find("Notional") != std::string::npos
            or pair.first.find("size") != std::string::npos
            or pair.first.find("Size") != std::string::npos
            or pair.first.find("grossValue") != std::string::npos
            or pair.first.find("Px") != std::string::npos) {
            // cast to double
            retVal[pair.first] = web::json::value(std::atof(pair.second.c_str()));
            retVal[pair.first].as_double();
        } else {
            // take it as string
            retVal[pair.first] = web::json::value(pair.second);
        }
    }
    // add timestamp if it is missing
    if (_data.find("timestamp") == _data.end()) {
        std::chrono::time_point<std::chrono::system_clock> time_now = std::chrono::system_clock::now();
        std::time_t time_now_t = std::chrono::system_clock::to_time_t(time_now);
        std::tm now_tm = *std::localtime(&time_now_t);
        char buf[256];
        std::strftime(buf, 256, "%yyyy-%m-%dT%H:%m:%s.%f", &now_tm);
        retVal["timestamp"] = web::json::value(buf);
    }
    return retVal;
}

const std::string &Params::at(const std::string &key_, const std::string &default_) {
    if (_data.find(key_) == _data.end()) {
        return default_;
    }
    return at(key_);
}
