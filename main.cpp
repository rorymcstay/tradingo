#include <iostream>
#include <string>
#include <filesystem>

#include "MarketData.h"
#include "Event.h"
#include "BatchWriter.h"
#include "InputParser.h"


using namespace io::swagger::client;
namespace ws = web::websockets;


int main(int argc, char **argv) {

    auto cliParser = std::make_shared<InputParser>(argc, argv);

    std::string connectionString = "wss://www.bitmex.com";
    std::string symbol = "XBTUSD";
    std::string storage = "/home/rory/dev/tradingo/";

    if (cliParser->cmdOptionExists("symbol"))
        symbol = cliParser->getCmdOption("symbol");
    if (cliParser->cmdOptionExists("connection"))
        connectionString = cliParser->getCmdOption("connection");
    if (cliParser->cmdOptionExists("storage"))
        storage = cliParser->getCmdOption("storage");

    auto marketData = std::make_shared<MarketData>(connectionString, symbol);

    // table writers
    auto trades = std::make_shared<BatchWriter>("trades", symbol, storage);
    auto instruments = std::make_shared<BatchWriter>("instruments", symbol, storage);
    auto quotes = std::make_shared<BatchWriter>("quotes", symbol, storage);

    while (marketData)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto data = marketData->read();
        if (data) {
            switch (data->eventType()) {case (EventType::BBO): {
                    auto qt = data->getQuote();
                    std::cout << "Quote[" << qt->toJson().serialize() << "]\n";
                    quotes->write(qt);
                    break;
                }
                case EventType::TradeUpdate: {
                    auto trd = data->getTrade();
                    std::cout << "Trade[" << trd->toJson().serialize() << "]\n";
                    trades->write(trd);
                    break;
                }
                case EventType::Instrument: {
                    auto instr = data->getInstrument();
                    // TODO: use base class
                    // auto http = model::ModelBase;
                    std::cout << "Instrument[" << instr->toJson().serialize() << "]\n";
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

