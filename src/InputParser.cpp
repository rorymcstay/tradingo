//
// Created by rory on 15/06/2021.
//

#include "InputParser.h"

InputParser::InputParser(int &argc, char **argv) {
    for (int i=1; i < argc; ++i)
        _tokens.push_back(std::string(argv[i]));
}

const std::string &InputParser::getCmdOption(const std::string &option) const {
    std::vector<std::string>::const_iterator itr;
    itr =  std::find(_tokens.begin(), _tokens.end(), option);
    if (itr != _tokens.end() && ++itr != _tokens.end()){
        return *itr;
    }
    static const std::string empty_string("");
    return empty_string;
}

bool InputParser::cmdOptionExists(const std::string &option) const {
    return std::find(_tokens.begin(), _tokens.end(), option) != _tokens.end();
}
