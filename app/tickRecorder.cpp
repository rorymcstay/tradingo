#include <iostream>
#include <string>
#include <filesystem>

#include <boost/program_options.hpp>

#include "MarketData.h"
#include "Event.h"
#include "BatchWriter.h"
#include "Utils.h"


using namespace io::swagger::client;
namespace ws = web::websockets;
namespace po = boost::program_options;

// TODO Move to app, src directory

int main(int argc, char **argv) {

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("storage", po::value<std::string>(), "where to store files")
            ("symbol", po::value<std::string>(), "symbol")
            ("config", po::value<std::string>(), "config file")
            ("connection", po::value<std::string>(), "base url of bitmex exchange");

    std::string connectionString = "wss://www.bitmex.com";
    std::string symbol = "XBTUSD";
    std::string storage = "/home/rory/dev/tradingo/";

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    std::shared_ptr<Config> config;
    if (vm.contains("config")) {
        config = std::make_shared<Config>("./config/trading.cfg");
        storage = config->get("storage");
        symbol = config->get("symbol");
        connectionString = config->get("webSocketUrl");
    } else {
        if (vm.contains("storage")) {
            storage = vm.at("storage").as<std::string>();
        }
        if (vm.contains("connection")) {
            connectionString = vm.at("connection").as<std::string>();
        }
        if (vm.contains("symbol")) {
            symbol = vm.at("symbol").as<std::string>();
        }
    }
    INFO("Starting tick recording with " << LOG_VAR(symbol) << LOG_VAR(connectionString) << LOG_VAR(storage));

    auto marketData = std::make_shared<MarketData>(config);

    // table writers
    auto trades = std::make_shared<BatchWriter>("trades", symbol, storage);
    auto instruments = std::make_shared<BatchWriter>("instruments", symbol, storage);
    auto quotes = std::make_shared<BatchWriter>("quotes", symbol, storage);

    while (marketData)
    {
        auto data = marketData->read();
        if (data) {
            switch (data->eventType()) {case (EventType::BBO): {
                    auto qt = data->getQuote();
                    quotes->write(qt);
                    break;
                }
                case EventType::TradeUpdate: {
                    auto trd = data->getTrade();
                    trades->write(trd);
                    break;
                }
                case EventType::Instrument: {
                    auto instr = data->getInstrument();
                    // TODO: use base class
                    // auto http = model::ModelBase;
                    instruments->write(instr);
                }
            }
        }
    }
    instruments->write_batch();
    trades->write_batch();
    quotes->write_batch();
    return 0;
}

