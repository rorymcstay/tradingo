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
    InputParser (int &argc, char **argv)
    {
        for (int i=1; i < argc; ++i)
            _tokens.push_back(std::string(argv[i]));
    }

    const std::string& getCmdOption(const std::string &option) const
    {
        std::vector<std::string>::const_iterator itr;
        itr =  std::find(_tokens.begin(), _tokens.end(), option);
        if (itr != _tokens.end() && ++itr != _tokens.end()){
            return *itr;
        }
        static const std::string empty_string("");
        return empty_string;
    }

    bool cmdOptionExists(const std::string &option) const
    {
        return std::find(_tokens.begin(), _tokens.end(), option) != _tokens.end();
    }

private:
    std::vector <std::string> _tokens;
};


#endif //TRADING_BOT_INPUTPARSER_H
