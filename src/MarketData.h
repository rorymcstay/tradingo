//
// Created by rory on 14/06/2021.
//

#ifndef TRADING_BOT_MARKETDATA_H
#define TRADING_BOT_MARKETDATA_H
#include <memory>
#include <thread>
#include <model/Trade.h>
#include <model/Instrument.h>
#include <model/Quote.h>
#include <model/Position.h>
#include <model/Order.h>
#include <model/Execution.h>
#include <model/Margin.h>
#include <mutex>
#include <cpprest/json.h>
#include <cpprest/ws_client.h>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>

#include "Event.h"
#include "Config.h"
#include "HeartBeat.h"
#include "Allocation.h"
#include "api/InstrumentApi.h"
#include "InstrumentService.h"
//#include "Signal.h"


using namespace io::swagger::client;
namespace ws = web::websockets;

std::string getPositionKey(const std::shared_ptr<model::Position>& pos_);

std::string getOrderKey(const std::shared_ptr<model::Order>& order_);

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
        // uncomment to use allocator pool
        auto quote = std::make_shared<T>();
        quote->fromJson(dataPiece);
        out_data_.push_back(quote);
    }
    return out_data_;
}


namespace ws = web::websockets;
using namespace io::swagger::client;


class MarketDataInterface {
    /// market data interface is a queue ontop of bitmex websocket.
    /// it has a read method and upto date attributes of the market
    /// status. consecutive events are read of the queue with the
    /// read method.

public:
    using OrderPtr = std::shared_ptr<model::Order>;
    using PositionPtr = std::shared_ptr<model::Position>;
    using TradePtr = std::shared_ptr<model::Trade>;
    using QuotePtr = std::shared_ptr<model::Quote>;
    using ExecPtr = std::shared_ptr<model::Execution>;
    using MarginPtr = std::shared_ptr<model::Margin>;

    std::function<void()> _callback;

    /// callback may be added which is evaluated on every update.
    void setCallback(const std::function<void()> &callback);

private:
    /// the event queue - trade execution or quote.
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, QueueArrange> _eventBuffer;
    //std::unordered_map<std::string, std::shared_ptr<Signal>> _timed_signals;

private:
    /// update the signals for event
    void updateSignals(const std::shared_ptr<Event>& event_);
public:
    /// initialise the signals
    void initSignals(const std::string& cfg_);

protected:
    std::mutex _mutex;
    std::vector<std::string> _positionKey;
    std::vector<std::string> _orderKey;
    std::string _symbol;

    std::queue<std::shared_ptr<model::Execution>> _executions;
    std::unordered_map<std::string, std::shared_ptr<model::Position>> _positions;
    std::unordered_map<std::string, std::shared_ptr<model::Order>> _orders;
    std::shared_ptr<model::Quote> _quote;
    std::shared_ptr<model::Margin> _margin;
    std::unordered_map<std::string, std::shared_ptr<model::Instrument>> _instruments;
    std::shared_ptr<InstrumentService> _instSvc;

    /// handle quote update after data is read from socket.
    void handleQuotes(const std::vector<std::shared_ptr<model::Quote>>& quotes_, const std::string& action_);
    /// handle trade update after data is read from socket.
    void handleTrades(std::vector<std::shared_ptr<model::Trade>>& trades_, const std::string& action_);
    /// handle positon update after data is read from socket.
    void handlePositions(std::vector<std::shared_ptr<model::Position>>& trades_, const std::string& action_);
    /// handle execution update after data is read from socket.
    void handleExecutions(std::vector<std::shared_ptr<model::Execution>>& execs_, const std::string& action_);
    /// handle order update after data is read from socket.
    void handleOrders(std::vector<std::shared_ptr<model::Order>>& orders_, const std::string& action_);
    /// handle updates to margin and balance
    void handleMargin(std::vector<std::shared_ptr<model::Margin>> margin_, const std::string& action_);
    /// handle instruments
    void handleInstruments(const std::vector<std::shared_ptr<model::Instrument>>& instruments_, const std::string& action_);
    /// evaluate callback linked to self. i.e signals
    void callback() {
        _callback();
    }
    /// load the instrument static data from instrument service.
    void init() {
    }

protected:
    template<typename T>
    /// specific update handlers, ie. delete
    void update(const std::vector<std::shared_ptr<T>>& data_);
    void updatePositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void insertPositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void removePositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void insertOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);

    void removeOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);
    void updateOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);

    void updateKey(const web::json::array& values_, std::vector<std::string>& key_);

public:
    ~MarketDataInterface() = default;
    MarketDataInterface(const std::shared_ptr<Config>& config_, std::shared_ptr<InstrumentService>  insSvc_);
    MarketDataInterface();
    /// get the next event.
    std::shared_ptr<Event> read();
    /// current open orders.
    const std::unordered_map<std::string, OrderPtr>& getOrders() const;
    /// latest executions.
    const std::queue<ExecPtr>& getExecutions() const;
    /// get current positions.
    const std::unordered_map<std::string, PositionPtr>& getPositions() const;
    /// get current margin.
    const MarginPtr& getMargin() const;
    /// get current quote.
    const std::shared_ptr<model::Quote> quote() const;
    /// get instrument static.
    const std::shared_ptr<model::Instrument>& instrument() const;
    const std::unordered_map<std::string, std::shared_ptr<model::Instrument>>& getInstruments() const;

};

using namespace io::swagger::client;

class MarketData
:   public MarketDataInterface
{
    /// Wrapper bitmex websocket.
    std::string _connectionString;
    std::shared_ptr<ws::client::websocket_callback_client> _wsClient;

    std::string _apiSecret;
    std::string _apiKey;
    bool _initialised;
    bool _shouldAuth;
    bool _cancelAllAfter;
    int _cancelAllTimeout;

    std::shared_ptr<HeartBeat> _heartBeat;
    long _cycle;

private:
    std::string getConnectionUri();
    std::string getBaseUrl();

public:
    void subscribe();
    void reconnect();

    MarketData(const std::shared_ptr<Config>& config_, std::shared_ptr<InstrumentService>);

    ~MarketData();

    void init();

};

#endif //TRADING_BOT_MARKETDATA_H
