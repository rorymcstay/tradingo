//
// Created by rory on 01/01/2022.
//

#include <functional>
#include <gtest/gtest.h>
#include <cpprest/json.h>
#include <boost/program_options.hpp>
#include <memory>
#include <sstream>
#include <unordered_map>

#include <pplx/threadpool.h>

#include <model/Position.h>
#include <model/Quote.h>
#include <model/Instrument.h>
#include <model/Margin.h>
#include <model/Order.h>
#include <model/Execution.h>
#include <stdexcept>

#include "ModelBase.h"
#include "fwk/TestEnv.h"
#include "Config.h"


using namespace io::swagger::client;
namespace json = web::json;
namespace po = boost::program_options;

std::string REPLAY_FILE;
std::string CONFIG_FILE;



TEST(Replay, scenario) {

    std::unordered_map<std::string, std::string> test_cl_ord_id_lookup;
    std::unordered_map<std::string, std::string> test_order_id_lookup;

    auto pplxThreadCount = 2;
    crossplat::threadpool::initialize_with_threads(pplxThreadCount);
    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({DEFAULT_ARGS}));
    auto env = TestEnv(defaults);
    if (CONFIG_FILE != "NONE") {
        auto config = std::make_shared<Config>(CONFIG_FILE);
        LOGINFO("Using config " << CONFIG_FILE);
        *defaults += (*config);
    }

    std::ifstream dataFile;

    dataFile.open(REPLAY_FILE);

    std::string str_buffer;
    auto json_buffer = json::value();
    while (std::getline(dataFile, str_buffer)) {
        if (str_buffer.empty())
            continue;
        json_buffer = web::json::value::parse(str_buffer);
        std::string object_type = json_buffer["object_type"].as_string();
        std::string event_type = json_buffer["event_type"].as_string();
        auto data = json_buffer["data"];
        std::stringstream description;
        if (json_buffer.has_string_field("description"))
            description << LOG_NVP("description", json_buffer["description"].as_string());
        LOGINFO("Event " LOG_VAR(object_type) 
                << LOG_VAR(event_type) 
                << description.str()); 
        if (object_type == "Position") {
            auto position = std::make_shared<model::Position>();
            position->fromJson(data);
            if (event_type == "IN_EVENT") {
                env << position;
            } else if (event_type == "ASSERTION") {
                env >> position;
            } else {
                throw std::runtime_error(event_type);
            }
        } else if (object_type == "Quote") {
            auto quote = std::make_shared<model::Quote>();
            quote->fromJson(data);
            env << quote;
        } else if (object_type == "Instrument") {
            auto instrument = std::make_shared<model::Instrument>();
            instrument->fromJson(data);
            env << instrument;
        } else if (object_type == "Margin") {
            auto margin = std::make_shared<model::Margin>();
            margin->fromJson(data);
            if (event_type == "IN_EVENT") {
                env << margin;
            } else if (event_type == "ASSERTION") {
                env >> margin;
            } else {
                throw std::runtime_error(event_type);
            }
        } else if (object_type == "Order") {
            auto order = std::make_shared<model::Order>();
            order->fromJson(data);
            auto test_order = env >> order;
            test_cl_ord_id_lookup[order->getClOrdID()] = test_order->getClOrdID();
            test_order_id_lookup[order->getOrderID()] = test_order->getOrderID();
        } else if (object_type == "Execution") {
            auto execution = std::make_shared<model::Execution>();
            execution->fromJson(data);
            execution->setClOrdID(test_cl_ord_id_lookup.at(execution->getClOrdID()));
            execution->setOrderID(test_order_id_lookup.at(execution->getOrderID()));
            env << execution;
        } else {
            throw std::runtime_error(object_type);
        }
    }
}


int main(int argc, char **argv) {

    // setup CLI parser
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config-file", po::value<std::string>(), "path to config")
            ("scenario-file", po::value<std::string>(), "Scenario file");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    REPLAY_FILE = vm.at("scenario-file").as<std::string>();
    CONFIG_FILE = vm.at("config-file").as<std::string>();
    return RUN_ALL_TESTS();
}
