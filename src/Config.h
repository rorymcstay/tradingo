//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_CONFIG_H
#define TRADINGO_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>


class Config {
    std::unordered_map<std::string, std::string> _data;

public:
    Config();
    explicit Config(const std::string& file_);
    std::string get(const std::string& name_ );
    void set(const std::string& key_, const std::string& val_);
};


#endif //TRADINGO_CONFIG_H
