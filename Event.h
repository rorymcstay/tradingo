//
// Created by rory on 14/06/2021.
//
#ifndef TRADING_BOT_EVENT_H
#define TRADING_BOT_EVENT_H

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

using namespace io::swagger::client;
namespace ws = web::websockets;

enum class EventType {TradeUpdate, BBO, Instrument};
enum class ActionType {Update, Delete, Insert, Partial};

class Event
{
private:
    std::shared_ptr<io::swagger::client::model::Instrument> _instrument;
    std::shared_ptr<io::swagger::client::model::Trade> _trade;
    std::shared_ptr<io::swagger::client::model::Quote> _quote;
    EventType _eventType;
    ActionType _actionType;
    std::chrono::system_clock::time_point _timeStamp;

public:
    Event(std::shared_ptr<model::Instrument> instr_)
            :   _instrument(std::move(instr_))
            ,   _trade(nullptr)
            ,   _quote(nullptr)
            ,   _eventType(EventType::Instrument)
    {}

    Event(std::shared_ptr<model::Trade> trade_)
            :   _instrument(nullptr)
            ,   _trade(std::move(trade_))
            ,   _quote(nullptr)
            ,   _eventType(EventType::TradeUpdate)
    {}
    Event(std::shared_ptr<model::Quote> quote_)
            :   _instrument(nullptr)
            ,   _trade(nullptr)
            ,   _quote(std::move(quote_))
            ,   _eventType(EventType::BBO)
    {}

    void setEventType(EventType eventType_) {
        _eventType = eventType_;
    }
    void setAction(const std::string& action_)
    {
        if (action_ == "update") {
            _actionType = ActionType::Update;
        } else if (action_ == "delete") {
            _actionType = ActionType::Delete;
        } else if (action_ == "insert") {
            _actionType = ActionType::Insert;
        } else if (action_ == "partial") {
            _actionType = ActionType::Partial;
        }
    }
    const std::shared_ptr<model::Instrument> &getInstrument() const {
        return _instrument;
    }
    const std::shared_ptr<model::Trade> &getTrade() const {
        return _trade;
    }
    const std::shared_ptr<model::Quote> &getQuote() const {
        return _quote;
    }
    // TODO operator <<
    EventType eventType() const { return _eventType; }
    std::chrono::system_clock::time_point timeStamp() { return _timeStamp; }

public:
};

#endif //TRADING_BOT_EVENT_H
