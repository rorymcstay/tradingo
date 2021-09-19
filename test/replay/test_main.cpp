
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"
#include <boost/program_options.hpp>
#include "Utils.h"
#include "Config.h"
#include "fwk/TestEnv.h"

#include "Utils.h"

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

    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"displaySize", "200"},
        {"referencePrice", "35000"},
        {"shortTermWindow", "1000"},
        {"longTermWindow", "8000"},
        {"logLevel", "info"},
        {"moving_average_crossover-interval", "1000"},
        {"signal-callback", "1000"},
        {"logLevel", "debug"},
        {"libraryLocation", LIBRARY_LOCATION"/libtest_trading_strategies.so"},

        {"storage", "./"}}));
    if (vm.contains("config")) {
        _config->set("libraryLocation", LIBRARY_LOCATION"/libtest_trading_strategies.so");
        auto config = std::make_shared<Config>(vm.at("config").as<std::string>());
        storage = config->get("storage");
        symbol = config->get("symbol");
        LOGINFO("Using config " << vm.at("config").as<std::string>());
        *defaults += (*config);
    }
    auto env = TestEnv(defaults);
    auto trade = defaults->get("tickStorage") + "/trades_XBTUSD.json";
    auto quotes = defaults->get("tickStorage") + "/quotes_XBTUSD.json";
    env.playback(trade, quotes);
    LOGINFO("Replaye Finished");
}


