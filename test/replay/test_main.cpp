//
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"
// cpprestsdk
#include <pplx/threadpool.h>
#include <boost/program_options.hpp>
#include "Utils.h"
#include "Config.h"
#include "Series.h"
#include "fwk/TestEnv.h"


namespace po = boost::program_options;

int main(int argc, char **argv) {


    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file")
            ("tick-storage", po::value<std::string>(), "override tick storage location in config.")
            ("logdir", po::value<std::string>(), "override logFilelocation in config.")
            ("storage", po::value<std::string>(), "override storage location in config.");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    std::string config;

    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({DEFAULT_ARGS}));
    if (vm.contains("config")) {
        auto config = std::make_shared<Config>(vm.at("config").as<std::string>());
        LOGINFO("Using config " << vm.at("config").as<std::string>());
        *defaults += (*config);
    }
    if (vm.contains("tick-storage")) {
        defaults->set("tickStorage", vm.at("tick-storage").as<std::string>());
    }
    if (vm.contains("storage")) {
        defaults->set("storage", vm.at("storage").as<std::string>());
    }
    if (vm.contains("logdir")) {
        defaults->set("logFileLocation", vm.at("logdir").as<std::string>());
    }
    auto pplxThreadCount = std::stoi(defaults->get("pplxThreadCount", "0"));
    crossplat::threadpool::initialize_with_threads(pplxThreadCount);
    auto env = TestEnv(defaults);
    auto trade = defaults->get("tickStorage") + "/trades_XBTUSD.json";
    auto quotes = defaults->get("tickStorage") + "/quotes_XBTUSD.json";
    auto instruments = defaults->get("tickStorage") + "/instruments_XBTUSD.json";
    env.playback(trade, quotes, instruments);
    LOGINFO("Replay Finished");
}
