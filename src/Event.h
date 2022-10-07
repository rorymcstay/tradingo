//
// Created by rory on 14/06/2021.
//
#ifndef TRADING_BOT_EVENT_H
#define TRADING_BOT_EVENT_H

#include <cpprest/asyncrt_utils.h>
#include <thread>
#include <model/OrderBookL2.h>
#include <model/Trade.h>
#include <model/Instrument.h>
#include <model/Execution.h>
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

enum class EventType {TradeUpdate, BBO, Instrument, Exec, OrderBookUpdate};
enum class ActionType {Update, Delete, Insert, Partial};

class Event
{
private:
    // TODO add execution, remove _instrument
    std::shared_ptr<io::swagger::client::model::Instrument> _instrument;
    std::shared_ptr<io::swagger::client::model::Instrument> _instrumentDelta;
    std::shared_ptr<io::swagger::client::model::Trade> _trade;
    std::shared_ptr<io::swagger::client::model::Quote> _quote;
    std::shared_ptr<io::swagger::client::model::Execution> _exec;
    std::shared_ptr<io::swagger::client::model::OrderBookL2> _orderbook;
    EventType _eventType;
    ActionType _actionType;
    utility::datetime _timeStamp;

public:
    Event(std::shared_ptr<model::Instrument> instr_,
        std::shared_ptr<model::Instrument> delta_)
            :   _instrument(std::move(instr_))
            ,   _instrumentDelta(std::move(delta_))
            ,   _trade(nullptr)
            ,   _quote(nullptr)
            ,   _exec(nullptr)
            ,   _orderbook(nullptr)
            ,   _timeStamp(_instrumentDelta->getTimestamp())
            ,   _eventType(EventType::Instrument) {
    }

    Event(std::shared_ptr<model::Trade> trade_)
            :   _instrument(nullptr)
            ,   _instrumentDelta(nullptr)
            ,   _trade(std::move(trade_))
            ,   _quote(nullptr)
            ,   _exec(nullptr)
            ,   _orderbook(nullptr)
            ,   _timeStamp(_trade->getTimestamp())
            ,   _eventType(EventType::TradeUpdate) {}

    Event(std::shared_ptr<model::Quote> quote_)
            :   _instrument(nullptr)
            ,   _instrumentDelta(nullptr)
            ,   _trade(nullptr)
            ,   _quote(std::move(quote_))
            ,   _exec(nullptr)
            ,   _orderbook(nullptr)
            ,   _timeStamp(_quote->getTimestamp())
            ,   _eventType(EventType::BBO) {}

    Event(std::shared_ptr<model::Execution> exec_)
            :   _instrument(nullptr)
            ,   _instrumentDelta(nullptr)
            ,   _trade(nullptr)
            ,   _quote(nullptr)
            ,   _orderbook(nullptr)
            ,   _exec(std::move(exec_))
            ,   _timeStamp(_exec->getTimestamp())
            ,   _eventType(EventType::Exec) {}

    Event(std::shared_ptr<model::OrderBookL2> ob_)
            :   _instrument(nullptr)
            ,   _instrumentDelta(nullptr)
            ,   _trade(nullptr)
            ,   _quote(nullptr)
            ,   _exec(nullptr)
            ,   _orderbook(std::move(ob_))
            ,   _timeStamp(ob_->getTimestamp())
            ,   _eventType(EventType::OrderBookUpdate) {}

    void setEventType(EventType eventType_) { _eventType = eventType_; }
    void setAction(const std::string& action_);
    const std::shared_ptr<model::Instrument>& getInstrument() const { return _instrument; }
    const std::shared_ptr<model::Instrument>& getInstrumentDelta() const { return _instrumentDelta; }
    const std::shared_ptr<model::Trade>& getTrade() const { return _trade; }
    const std::shared_ptr<model::Quote>& getQuote() const { return _quote; }
    const std::shared_ptr<model::Execution>& getExec() const { return _exec; }
    const std::shared_ptr<model::OrderBookL2>& getBookUpdate() const { return _orderbook; }
    EventType eventType() const { return _eventType; }
    utility::datetime timeStamp() { return _timeStamp; }

};

#endif //TRADING_BOT_EVENT_H
