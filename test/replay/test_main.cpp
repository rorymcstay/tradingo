
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"
#include <boost/program_options.hpp>
#include "Utils.h"
#include "Config.h"
#include "fwk/TestEnv.h"

namespace po = boost::program_options;


// TODO Add command line interface for config file

int main(int argc, char **argv) {

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    std::string storage;
    std::string config;
    std::string symbol;

    if (vm.contains("config")) {
        auto config = std::make_shared<Config>(vm.at("config").as<std::string>());
        storage = config->get("storage");
        symbol = config->get("symbol");
        LOGINFO("Using config " << vm.at("config").as<std::string>());
    }
    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({{"symbol", "XBTUSD"},
    {"clOrdPrefix", "MCST"},
    {"factoryMethod", "RegisterBreakOutStrategy"},
    {"startingAmount", "1000"},
    {"displaySize", "200"},
    {"referencePrice", "35000"},
    {"shortTermWindow", "10000"},
    {"longTermWindow", "50000"},
    {"logLevel", "info"},
    {"storage", "/home/tradingo/replays/"}}));
    auto dft = (defaults);
    *defaults += (*defaults);
    auto env = TestEnv(defaults);
    env.playback("trades_XBTUSD.json", "quotes_XBTUSD.json");

}


