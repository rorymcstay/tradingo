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
        INFO(LOG_VAR(file_));
        while(getline(cFile, line)){
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                       line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find('=');
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            INFO("Reading config: " << LOG_VAR(name) << LOG_VAR(value) << '\n');
            set(name, value);
        }
    }
    else {
        std::stringstream error;
        error << "Couldn't open config " << LOG_NVP("file", file_) << " for reading.\n";
        throw std::runtime_error(error.str());
    }

}

std::string Config::get(const std::string &name_) {
    return _data.at(name_);
}

void Config::set(const std::string& key_, const std::string& val_) {
    _data[key_] = val_;
}