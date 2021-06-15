//
// Created by rory on 14/06/2021.
//

#ifndef TRADING_BOT_MARKETDATA_H
#define TRADING_BOT_MARKETDATA_H
#include <thread>
#include <model/Trade.h>
#include <model/Instrument.h>
#include <model/Quote.h>
#include <mutex>
#include <cpprest/json.h>
#include <cpprest/ws_client.h>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>
#include "Event.h"

using namespace io::swagger::client;
namespace ws = web::websockets;

class QueueArrange {
public:
    bool operator()(const std::shared_ptr<Event> &this_, const std::shared_ptr<Event> &that_) {
        return this_->timeStamp() < that_->timeStamp(); //fifo
    }
};

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

namespace ws = web::websockets;
using namespace io::swagger::client;
class MarketData
{
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, QueueArrange> _eventBuffer;
    std::mutex _mutex;
    std::string _connectionString;
    std::string _symbol;

    std::shared_ptr<ws::client::websocket_callback_client> _wsClient;
    bool _initialised;

private:
    template<typename T>
    void update(const std::vector<std::shared_ptr<T>>& data_);
    template<typename T>
    void remove(const std::vector<std::shared_ptr<T>>& data_);
public:

    explicit MarketData(std::string connectionString_, std::string symbol_);
    ~MarketData();
    std::string getConnectionString();
    std::shared_ptr<Event> read();
};

#endif //TRADING_BOT_MARKETDATA_H
