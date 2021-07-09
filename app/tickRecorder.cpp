#include <iostream>
#include <string>
#include <filesystem>

#include <boost/program_options.hpp>
#include <api/OrderApi.h>

#include "MarketData.h"
#include "Event.h"
#include "BatchWriter.h"
#include "Utils.h"
#include "Context.h"


using namespace io::swagger::client;
namespace ws = web::websockets;
namespace po = boost::program_options;

int main(int argc, char **argv) {

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    std::shared_ptr<Config> config;
    std::string symbol;
    std::string storage;
    if (vm.contains("config")) {
        config = std::make_shared<Config>(vm.at("config").as<std::string>());
        storage = config->get("storage");
        symbol = config->get("symbol");
    } else {
        return 1;
    }
    auto context = std::make_shared<Context<MarketData, io::swagger::client::api::OrderApi>>(config);
    context->init();

    auto marketData = context->marketData();

    LOGINFO("Starting tick recording with " << LOG_VAR(symbol) << LOG_VAR(storage));

    // table writers
    auto trades = std::make_shared<BatchWriter>("trades", symbol, storage);
    auto quotes = std::make_shared<BatchWriter>("quotes", symbol, storage);

    while (marketData)
    {
        auto data = marketData->read();
        if (data) {
            switch (data->eventType()) {
                case (EventType::BBO): {
                    auto qt = data->getQuote();
                    quotes->write(qt);
                    break;
                }
                case EventType::TradeUpdate: {
                    auto trd = data->getTrade();
                    trades->write(trd);
                    break;
                }
            }
        }
    }
    trades->write_batch();
    quotes->write_batch();
    return 0;
}

