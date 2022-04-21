//
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"
// cpprestsdk
#include <boost/program_options/errors.hpp>
#include <cpprest/asyncrt_utils.h>
#include <cpprest/json.h>
#include <exception>
#include <memory>
#include <pplx/threadpool.h>
#include <boost/program_options.hpp>
#include <sstream>
#include "Utils.h"
#include "Config.h"
#include "Series.h"
#include "fwk/TestEnv.h"
#include "model/Margin.h"
#include "model/Position.h"


namespace po = boost::program_options;




int main(int argc, char **argv) {


    po::options_description desc("Allowed options");
    std::vector<std::string> config_files;
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::vector<std::string>>(&config_files), "config file, may be specified multiple times")
            ("config-json", po::value<std::string>(), "config in json string")
            ("tick-storage", po::value<std::string>(), "override tick storage location in config.")
            ("logdir", po::value<std::string>(), "override logFilelocation in config.")
            ("trade-resolution", po::value<long>(), "trade time resolution for series")
            ("instrument-resolution", po::value<long>(), "instrument time resolution for series")
            ("quote-resolution", po::value<long>(), "quote time resolution for series")
            ("storage", po::value<std::string>(), "override storage location in config.")
            ("initial-margin", po::value<std::string>(), "override the initial margin object")
            ("initial-position", po::value<std::string>(), "override the initial position")
            ("start-time", po::value<std::string>(), "the time to start replay from")
            ("end-time", po::value<std::string>(), "the time to finish  replay from")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    try {
        po::notify(vm);
    } catch (const po::error& e ) {
        LOGERROR("Error parsing program options: " << e.what());
        return 1;
    }

    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({DEFAULT_ARGS}));
    if (not config_files.empty()) {
        for (auto& filepath : config_files) {
            auto config = std::make_shared<Config>(filepath);
            LOGINFO("Applying config " << filepath);
            *defaults += (*config);
        }
    }
    if (vm.contains("config-json")) {
        auto json_str = vm.at("config-json").as<std::string>();
        web::json::value json_val;
        try {
            LOGINFO("Applying config overrides");
            json_val = web::json::value::parse(json_str);
        } catch (const std::exception& ex) {
            std::stringstream errmsg;
            errmsg << "Invalid config dict passed"
                << LOG_VAR(ex.what()) << LOG_VAR(json_str);
            LOGERROR(errmsg.str());
            return 1;
        }
        auto config = std::make_shared<Config>(json_val);
        LOGINFO("Using config " << vm.at("config-json").as<std::string>());
        *defaults += (*config);
    }
    LOGINFO("Config: " << defaults->toJson().serialize());
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
    auto trades_file = defaults->get<std::string>("tickStorage") + "/trades_XBTUSD.json";
    auto quotes_file = defaults->get<std::string>("tickStorage") + "/quotes_XBTUSD.json";
    auto instruments_file = defaults->get<std::string>("tickStorage") + "/instruments_XBTUSD.json";

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

    auto initial_margin = std::shared_ptr<model::Margin>();
    if (vm.contains("initial-margin")) {
        initial_margin = std::make_shared<model::Margin>();
        auto json_str = vm.at("initial-margin").as<std::string>();
        web::json::value json_val;
        try {
            json_val = web::json::value::parse(json_str);
        } catch (const std::exception& ex) {
            std::stringstream errmsg;
            errmsg << "Invalid initial margin passed"
                << LOG_VAR(ex.what()) << LOG_VAR(json_str);
            LOGERROR(errmsg.str());
            return 1;
        }
        initial_margin->fromJson(json_val);
        LOGINFO("Read initial margin from command line "
                << initial_margin->toJson().serialize());
    }
    auto initial_position = std::shared_ptr<model::Position>();
    if (vm.contains("initial-position")) {
        initial_position = std::make_shared<model::Position>();
        auto json_str = vm.at("initial-position").as<std::string>();
        web::json::value json_val;
        try {
            json_val = web::json::value::parse(json_str);
        } catch (const std::exception& ex) {
            std::stringstream errmsg;
            errmsg << "Invalid initial position passed"
                << LOG_VAR(ex.what()) << LOG_VAR(json_str);
            LOGERROR(errmsg.str());
            return 1;
        }
        initial_position->fromJson(json_val);
        LOGINFO("Read initial position from command line "
                    << initial_position->toJson().serialize());
    }

    auto quotes = Series<model::Quote>(quotes_file, quote_resolution);
    auto trades = Series<model::Trade>(trades_file, trade_resolution);
    auto instruments = Series<model::Instrument>(instruments_file, instrument_resolution);
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
    if (initial_margin)
        env << initial_margin;
        LOGINFO("Applied initial margin from command line");
    if (initial_position)
        env << initial_position;
        LOGINFO("Applied initial position from command line");
    env.playback(trades, quotes, instruments);
    LOGINFO("Replay Finished");
}
