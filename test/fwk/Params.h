//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_PARAMS_H
#define TRADINGO_PARAMS_H

#include <map>
#include <cpprest/json.h>
#include "Utils.h"

using Params_data = std::map<std::string, std::string>;

template<typename TParams>
static void parse_params_string(const std::string& msgStr_, TParams& params_)
{
    std::string chunk;
    std::istringstream iss(msgStr_);
    while(std::getline(iss, chunk, ' '))
    {
        size_t pos = chunk.find('=');
        if (pos == std::string::npos)
            continue;
        std::string key = chunk.substr(0, pos);
        std::string val = chunk.substr(pos+1);
        params_[key] = val;
    }
}

class Params
{
    Params_data _data;
    std::string _paramString;
public:
    Params()= default;
    explicit Params(std::string  str_);
    std::string& operator[] (const std::string& key_);
    std::string& at(const std::string& key_);
    web::json::value asJson() const;
    const std::string& at(const std::string& key_, const std::string& default_);
};

#endif //TRADINGO_PARAMS_H
