#include <iostream>
#include <string>
#include <filesystem>

// cpprestsdk
#include <pplx/threadpool.h>

#include <boost/program_options.hpp>
#include <api/OrderApi.h>
#include <api/PositionApi.h>

#include "MarketData.h"
#include "Event.h"
#include "BatchWriter.h"
#include "Utils.h"
#include "Context.h"


using namespace io::swagger::client;
namespace ws = web::websockets;
namespace po = boost::program_options;

using ModelBatchWriter = BatchWriter<std::shared_ptr<model::ModelBase>>;

bool should_run(std::chrono::system_clock::time_point started_at_) {
    auto started_at_t = std::chrono::system_clock::to_time_t(started_at_);
    auto current_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto started_at = localtime(&started_at_t);
    auto current = localtime(&current_t);
    auto current_day = current->tm_mday;
    auto started_day = started_at->tm_mday;
    if (current_day != started_day)
        return false;
    return true;

}

int main(int argc, char **argv) {

    auto started_at = std::chrono::system_clock::now();
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
    auto context = std::make_shared<Context<MarketData, io::swagger::client::api::OrderApi, io::swagger::client::api::PositionApi>>(config);
    context->init();

    auto marketData = context->marketData();

    LOGINFO("Starting tick recording with " << LOG_VAR(symbol) << LOG_VAR(storage));

    auto pplxThreadCount = std::stoi(config->get("pplxThreadCount", "4"));
    crossplat::threadpool::initialize_with_threads(pplxThreadCount);

    // table writers
    auto printer = [](const std::shared_ptr<model::ModelBase>& order_) {
        return order_->toJson().serialize();
    };
    // enable file rotation for batch writers
    auto trades = std::make_shared<ModelBatchWriter>(
        "trades",
        symbol,
        storage,
        std::stoi(config->get("tradesBatchSize", "100")),
        printer,
        false);
    auto quotes = std::make_shared<ModelBatchWriter>(
        "quotes",
        symbol,
        storage,
        std::stoi(config->get("quotesBatchSize", "1000")),
        printer,
        false);
    auto instruments = std::make_shared<ModelBatchWriter>(
        "instruments",
        symbol,
        storage,
        std::stoi(config->get("instrumentsBatchSize", "1000")),
        printer,
        false);

    // only run for the current date
    while (should_run(started_at)) {
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
                case EventType::Instrument: {
                    auto inst = data->getInstrumentDelta();
                    instruments->write(inst);
                    break;
                }
            }
        }
    }
    trades->write_batch();
    quotes->write_batch();
    instruments->write_batch();
    return 0;
}