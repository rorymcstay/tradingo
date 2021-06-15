#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <cpprest/ws_client.h>
#include <cpprest/json.h>
#include <mutex>
#include <ranges>

#include <model/Quote.h>
#include <model/Instrument.h>
#include <filesystem>
#include <model/Trade.h>
#include <thread>
#include <iomanip>

#include "absl/strings/str_join.h"

#include "MarketData.h"
#include "Event.h"
#include "BatchWriter.h"


using namespace io::swagger::client;
namespace ws = web::websockets;



template<typename T>
std::vector<std::shared_ptr<T>>  getData(web::json::array& data_) {
    std::vector<std::shared_ptr<T>> out_data_;
    out_data_.reserve(data_.size());
    for (auto &dataPiece : data_) {
        // TODO: Use object pool
        auto quote = std::make_shared<T>();

        quote->fromJson(dataPiece);
        out_data_.push_back(quote);
    }
    return out_data_;
}

using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;

int main() {
    auto wsclient = std::make_shared<ws::client::websocket_callback_client>();

    auto marketData = std::make_shared<MarketData>();
    std::string date_string = formatTime(std::chrono::system_clock::now());
    std::string symbol = "XBTUSD";
    auto trades = std::make_shared<BatchWriter>("trades", symbol, "/home/rory/dev/tradingo/");
    auto instruments = std::make_shared<BatchWriter>("instruments", symbol, "/home/rory/dev/tradingo/");
    auto quotes = std::make_shared<BatchWriter>("quotes", symbol, "/home/rory/dev/tradingo/");

    wsclient->set_message_handler(
            [&marketData](const ws::client::websocket_incoming_message& in_msg) {
                auto msg = in_msg.extract_string();

                auto stringVal = msg.get();
                //std::cout << stringVal << std::endl;
                web::json::value msgJson = web::json::value::parse(stringVal);
                if (msgJson.has_field("info")) return;
                if (msgJson.has_field("success")) return;

                const std::string& table = msgJson.at("table").as_string();
                const std::string& action = msgJson.at("action").as_string();
                web::json::array& data = msgJson.at("data").as_array();
                if (table == "quote") {
                    auto qts = getData<model::Quote>(data);
                    marketData->update(qts);
                } else if (table == "trade") {
                    auto trds = getData<model::Trade>(data);
                    marketData->update(trds);
                }
            });
    wsclient->connect("wss://www.bitmex.com/realtime?subscribe=trade:XBTUSD,quote:XBTUSD");
    auto msg = ws::client::websocket_outgoing_message();
    wsclient->set_close_handler(
            [](ws::client::websocket_close_status stat, const std::string& message_, std::error_code err_) { std::cout << message_ << '\n';}
            );

    // TODO record date change to write to new file
    size_t date;

    while (wsclient)
    {
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        auto data = marketData->read();
        if (data)
        {
            switch (data->eventType())
            {
                case (EventType::BBO):
                {
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
                case EventType::Instrument:
                    auto instr = data->getInstrument();
                    // TODO: use base class
                    // auto http = model::ModelBase;
                    std::cout << "Instrument[" << instr->toJson().serialize() << "]\n";
                    instruments->write(instr);
            }
        }
    }
    instruments->write_batch();
    trades->write_batch();
    quotes->write_batch();
    return 0;
}

