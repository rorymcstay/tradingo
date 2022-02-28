//
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"
// cpprestsdk
#include <cpprest/asyncrt_utils.h>
#include <pplx/threadpool.h>
#include <boost/program_options.hpp>
#include "Utils.h"
#include "Config.h"
#include "Series.h"
#include "fwk/TestEnv.h"
#include "model/Margin.h"
#include "model/Position.h"


namespace po = boost::program_options;

/// get test meta file
template<typename T>
std::shared_ptr<T> get_object_from_file(const std::string& file_name) {
    std::string json_file = TESTDATA_LOCATION "/objects/" + file_name + ".json";
    std::ifstream t(json_file);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string val = buffer.str();
    auto out = std::make_shared<T>();
    auto json_val = web::json::value::parse(val);
    out->fromJson(json_val);
    return out;
}

int main(int argc, char **argv) {


    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file")
            ("tick-storage", po::value<std::string>(), "override tick storage location in config.")
            ("logdir", po::value<std::string>(), "override logFilelocation in config.")
            ("trade-resolution", po::value<long>(), "trade time resolution for series")
            ("instrument-resolution", po::value<long>(), "instrument time resolution for series")
            ("quote-resolution", po::value<long>(), "quote time resolution for series")
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

    auto pplxThreadCount = 1;
    crossplat::threadpool::initialize_with_threads(pplxThreadCount);
    auto env = TestEnv(defaults);
    auto trades_file = defaults->get("tickStorage") + "/trades_XBTUSD.json";
    auto quotes_file = defaults->get("tickStorage") + "/quotes_XBTUSD.json";
    auto instruments_file = defaults->get("tickStorage") + "/instruments_XBTUSD.json";

    auto trade_resolution = 10000;
    if (vm.contains("trade-resolution")) {
        trade_resolution = vm.at("trade-resolution").as<long>();
    }
    auto quote_resolution = 10000;
    if (vm.contains("quote-resolution")) {
        quote_resolution = vm.at("quote-resolution").as<long>();
    }
    auto instrument_resolution = 10000;
    if (vm.contains("instrument-resolution")) {
        instrument_resolution = vm.at("instrument-resolution").as<long>();
    }

    auto quotes = Series<model::Quote>(quotes_file, 10000);
    auto trades = Series<model::Trade>(trades_file, 10000);
    auto instruments = Series<model::Instrument>(instruments_file, 10000);
    if (vm.contains("start-time")) {
        quotes.set_start(vm.at("start-time").as<std::string>());
        trades.set_start(vm.at("start-time").as<std::string>());
        instruments.set_start(vm.at("start-time").as<std::string>());
    }
    if (vm.contains("end-time")) {
        quotes.set_end(vm.at("end-time").as<std::string>());
        trades.set_end(vm.at("end-time").as<std::string>());
        instruments.set_end(vm.at("end-time").as<std::string>());
    }
    if (quotes.begin()->getTimestamp().to_interval() > quotes.end()->getTimestamp().to_interval()) {
        std::stringstream ss;
        ss << "Invalid start and end times: start="
            << quotes.begin()->getTimestamp().to_string(utility::datetime::date_format::ISO_8601)
            << " end=" <<  quotes.end()->getTimestamp().to_string(utility::datetime::date_format::ISO_8601);
        throw std::runtime_error(ss.str());
    }
    env << get_object_from_file<model::Margin>("opening_margin");
    env << get_object_from_file<model::Position>("opening_position");
    env.playback(trades, quotes, instruments);
    LOGINFO("Replay Finished");
}
