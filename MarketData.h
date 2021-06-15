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
        return this_->timeStamp() > that_->timeStamp();
    }
};

class MarketData
{
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, QueueArrange> _eventBuffer;
    std::mutex _mutex;

public:

    template<typename T>
    void update(const std::vector<std::shared_ptr<T>>& data_) {
        std::lock_guard<decltype(_mutex)> lock(_mutex);
        for (const auto& row : data_)
            _eventBuffer.push(std::make_shared<Event>(row));
    }
    template<typename T>
    void remove(const std::vector<std::shared_ptr<T>>& data_) {};

    std::shared_ptr<Event> read() {
        std::lock_guard<decltype(_mutex)> lock(_mutex);
        auto event = _eventBuffer.empty() ? nullptr : _eventBuffer.top();
        if (event) {
            std::cout << "event_queue.size()=" << _eventBuffer.size() << "    ";
            _eventBuffer.pop();
        }
        return event;
    }
};

#endif //TRADING_BOT_MARKETDATA_H
