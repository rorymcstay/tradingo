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
        auto key = pair.first;
        key[0] = tolower(key[0]);

        // TODO Mandatory params
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
            retVal[key] = web::json::value(std::atof(pair.second.c_str()));
            retVal[key].as_double();
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
