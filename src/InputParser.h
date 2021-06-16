//
// Created by rory on 15/06/2021.
//

#ifndef TRADING_BOT_INPUTPARSER_H
#define TRADING_BOT_INPUTPARSER_H

#include <string>
#include <vector>

class InputParser
{
public:
    InputParser (int &argc, char **argv);
    const std::string& getCmdOption(const std::string &option) const;
    bool cmdOptionExists(const std::string &option) const;

private:
    std::vector <std::string> _tokens;
};


#endif //TRADING_BOT_INPUTPARSER_H
