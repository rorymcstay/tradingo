//
// Created by Rory McStay on 19/06/2021.
//

#include "Config.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include "Utils.h"


Config::Config(const std::string& file_)
:   _data() {
    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile (file_);
    if (cFile.is_open()) {
        std::string line;
        LOGINFO(LOG_VAR(file_));
        while(getline(cFile, line)){
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                       line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find('=');
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            LOGINFO("Reading config: " << LOG_VAR(name) << LOG_VAR(value) );
            set(name, value);
        }
    }
    else {
        std::stringstream error;
        error << "Couldn't open config " << LOG_NVP("file", file_) << " for reading.";
        LOGINFO(error.str());
        throw std::runtime_error(error.str());
    }

}

std::string Config::get(const std::string &name_) {
    try {
        return _data.at(name_);
    } catch (const std::exception& ex_) {
        std::stringstream message;
        message << "Missing Configuiration value "<< LOG_NVP("name", name_);
        LOGINFO(message.str());
        throw std::runtime_error( message.str() );
    }
}

void Config::set(const std::string& key_, const std::string& val_) {
    _data[key_] = val_;
}

Config::Config() : _data() {

}

Config::Config(std::initializer_list<std::pair<std::string, std::string>> kvps)
:   _data() {
    for (auto& kvp : kvps) {
        set(kvp.first, kvp.second);
    }
}

std::string Config::get(const std::string &name_, const std::string &default_) {
    if (_data.find(name_) == _data.end()) {
        LOGWARN("Defaulting " << name_ << " to " << default_);
        return default_;
    }
    else {
        return _data[name_];
    }
}
