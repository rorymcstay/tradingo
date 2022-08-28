//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_CONFIG_H
#define TRADINGO_CONFIG_H

#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <cctype>
#include <regex>
#include <algorithm>
#include <utility>
#include <cpprest/json.h>

#include "Utils.h"


class Config {

    web::json::object _data;

public:
    /// make empty config
    Config();
    /// from initialiser pairs list, used in tests.
    /// TestEnv ({{"val1", "1"}})
    Config(std::initializer_list<std::pair<std::string,std::string>>);
    Config(const web::json::value& json_);
    explicit Config(const std::string& file_);
    template<typename T> T get(const std::string& name_);
    template<typename T> T get(const std::string& name_, T default_);
    void set(const std::string& key_, const std::string& val_);
    void operator+=(const Config& config_);
    bool empty() const { return _data.empty(); }
    web::json::value toJson() const;
};


web::json::value read_json_file(const std::string& file_name);

template<typename T> T get_val(const web::json::object& json_, const std::string& key_);

inline
Config::Config(const web::json::value& value_)
:   _data(value_.as_object()) 
{}


inline
Config::Config(const std::string& file_)
: Config() {

    std::regex json_file_regex(".json$");
    std::regex flat_file_regex(".cfg$");
    if (std::regex_search( file_, json_file_regex)) {
        _data = read_json_file(file_).as_object();
        return;
    } else if (std::regex_search(file_, flat_file_regex)) {
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
    } else {
        throw std::runtime_error("Uknown config file type: " + file_);
    }

}

inline
web::json::value Config::toJson() const {
    auto out = web::json::value::parse("{}");
    for (auto& val : _data) {
        out.as_object()[val.first] = val.second;
    }
    return out;
}


inline
void Config::set(const std::string& key_, const std::string& val_) {
    _data[key_] = web::json::value(val_);
}


inline
Config::Config()
:    _data(web::json::value::parse("{}").as_object()) {
}


inline
Config::Config(std::initializer_list<std::pair<std::string, std::string>> kvps)
:   Config() {
    for (auto& kvp : kvps) {
        set(kvp.first, kvp.second);
    }
}

inline
void Config::operator+=(const Config &config_) {
    if (config_.empty())
        return;
    for (auto& kvp : config_._data) {
        if (_data.find(kvp.first) != _data.end())
            LOGWARN("Overriding orignal value " << LOG_NVP(kvp.first, _data[kvp.first])
                    << " with " << LOG_VAR(kvp.second));
        _data[kvp.first] = kvp.second;
    }
}


inline
web::json::value read_json_file(const std::string& file_name) {
    std::ifstream t(file_name);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string val = buffer.str();
    auto json_val = web::json::value::parse(val);
    return json_val;
}

template<>
inline
bool get_val(const web::json::object& json_, const std::string& key_) {

    if (json_.at(key_).is_string()) {
        auto val = json_.at(key_).as_string();
        std::transform(val.begin(), val.end(), val.begin(),
                [](unsigned char c) { return std::tolower(c); });
        if (val == "true") {
            return true;
        } else if (val == "false") {
            return false;
        } else if (val == "yes") {
            return true;
        } else if (val == "no") {
            return false;
        }
    }
    return json_.at(key_).as_bool();
}


template<>
inline
int get_val(const web::json::object& json_, const std::string& key_) {
    if (json_.at(key_).is_string()) {
        return std::stoi(json_.at(key_).as_string());
    }
    if (json_.at(key_).is_double()) {
        return (int)(json_.at(key_).as_double());
    }
    return json_.at(key_).as_integer();
}


template<>
inline
double get_val(const web::json::object& json_, const std::string& key_) {
    if (json_.at(key_).is_string()) {
        return std::atof(json_.at(key_).as_string().c_str());
    }
    if (json_.at(key_).is_integer()) {
        return (double)(json_.at(key_).as_integer());
    }
    return json_.at(key_).as_double();
}

template<>
inline
std::vector<std::string> get_val(const web::json::object& json_, const std::string& key_) {
    std::vector<std::string> out_vec;
    for (auto& val : json_.at(key_).as_array())
        out_vec.push_back(val.as_string());
    return out_vec;
}


template<>
inline
std::string get_val(const web::json::object& json_, const std::string& key_) {
    return json_.at(key_).as_string();
}


template<typename T>
inline
T Config::get(const std::string &name_) {
    try {
        return get_val<T>(_data, name_);
    } catch (const web::json::json_exception& ex_) {
        std::stringstream message;
        message << "Invalid configuration value "
            << LOG_NVP("name", name_)
            << LOG_NVP("what", ex_.what());
        LOGINFO(message.str());
        throw std::runtime_error( message.str() );
    }
}

template<>
inline
std::vector<std::string> Config::get(const std::string &name_) {
    try {
        return get_val<std::vector<std::string>>(_data, name_);
    } catch (const web::json::json_exception& ex_) {
        std::stringstream message;
        message << "Invalid configuration value "
            << LOG_NVP("name", name_)
            << LOG_NVP("what", ex_.what());
        LOGINFO(message.str());
        throw std::runtime_error( message.str() );
    }
}


template<typename T>
inline
T Config::get(const std::string &name_, T default_) {
    if (_data.find(name_) == _data.end()) {
        LOGWARN("Defaulting " << name_);
        return std::move(default_);
    }
    else {
        return get<T>(name_);
    }
}



#endif //TRADINGO_CONFIG_H
