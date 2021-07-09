//
// Created by rory on 14/06/2021.
//

#ifndef TRADING_BOT_MARKETDATA_H
#define TRADING_BOT_MARKETDATA_H
#include <thread>
#include <model/Trade.h>
#include <model/Instrument.h>
#include <model/Quote.h>
#include <model/Position.h>
#include <model/Order.h>
#include <model/Execution.h>
#include <mutex>
#include <cpprest/json.h>
#include <cpprest/ws_client.h>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>

#include "Event.h"
#include "ObjectPool.h"
#include "Config.h"
#include "HeartBeat.h"
#include "Allocation.h"
#include "api/InstrumentApi.h"
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


template<typename T, typename ObjPool>
std::vector<std::shared_ptr<T>>  getData(web::json::array& data_, ObjPool& pool_) {
    std::vector<std::shared_ptr<T>> out_data_;
    out_data_.reserve(data_.size());
    for (auto &dataPiece : data_) {
        // uncomment to use allocator pool
        //std::shared_ptr<T> quote = pool_.get();
        auto quote = std::make_shared<T>();
        quote->fromJson(dataPiece);
        out_data_.push_back(quote);
    }
    return out_data_;
}

struct TradeReleaser {

   void operator () (model::Trade* model_) {
        model_->unsetForeignNotional();
        model_->unsetGrossValue();
        model_->unsetHomeNotional();
        model_->unsetPrice();
        model_->unsetSide();
        model_->unsetSize();
        model_->unsetTickDirection();
        model_->unsetTrdMatchID();
    }
};
struct QuoteReleaser {

    void operator () (model::Quote* model_) {
        model_->unsetAskPrice();
        model_->unsetAskSize();
        model_->unsetBidPrice();
        model_->unsetBidSize();
    }
};

struct PositionReleaser {
    void operator () (model::Position* model_) {
        // must be a better way
        model_->unsetAvgCostPrice();
        model_->unsetAvgEntryPrice();
        model_->unsetBankruptPrice();
        model_->unsetBreakEvenPrice();
        model_->unsetCommission();
        model_->unsetCrossMargin();
        model_->unsetCurrentComm();
        model_->unsetCurrentCost();
        model_->unsetCurrentTimestamp();
        model_->unsetDeleveragePercentile();
        model_->unsetExecBuyCost();
        model_->unsetExecBuyQty();
        model_->unsetExecSellCost();
        model_->unsetExecSellQty();
        model_->unsetForeignNotional();
        model_->unsetGrossExecCost();
        model_->unsetIsOpen();
        model_->unsetUnrealisedTax();
        model_->unsetVarMargin();
    }
};

struct ExecutionReleaser {
    void operator() (model::Execution* exec_) {
        exec_->unsetWorkingIndicator();
        exec_->unsetForeignNotional();
        exec_->unsetCommission();
        exec_->unsetAccount();
        exec_->unsetAvgPx();
        exec_->unsetClOrdID();
        exec_->unsetClOrdLinkID();
        exec_->unsetContingencyType();
        exec_->unsetCumQty();
        exec_->unsetCurrency();
    }
};

struct OrderReleaser {
    void operator() (model::Order* order_) {
        //
    }
};

namespace ws = web::websockets;
using namespace io::swagger::client;


class MarketDataInterface {

public:
    using OrderPtr = std::shared_ptr<model::Order>;
    using PositionPtr = std::shared_ptr<model::Position>;
    using TradePtr = std::shared_ptr<model::Trade>;
    using QuotePtr = std::shared_ptr<model::Quote>;
    using ExecPtr = std::shared_ptr<model::Execution>;

private:
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, QueueArrange> _eventBuffer;
    //std::unordered_map<std::string, std::shared_ptr<Signal>> _signals;
private:
    void updateSignals(const std::shared_ptr<Event>& event_);
public:
    void initSignals(const std::string& cfg_);
    void setInstrumentApi(const std::shared_ptr<api::InstrumentApi> &instrumentApi);

protected:
    std::mutex _mutex;
    std::vector<std::string> _positionKey;
    std::vector<std::string> _orderKey;

    cache::ObjectPool<model::Trade, 1000, TradeReleaser> _tradePool;
    cache::ObjectPool<model::Quote, 1000, QuoteReleaser> _quotePool;
    cache::ObjectPool<model::Position, 1000, PositionReleaser> _positionPool;
    cache::ObjectPool<model::Execution, 1000, ExecutionReleaser> _execPool;
    cache::ObjectPool<model::Order, 1000, OrderReleaser> _orderPool;

    std::queue<std::shared_ptr<model::Execution>> _executions;
    std::unordered_map<std::string, std::shared_ptr<model::Position>> _positions;
    std::unordered_map<std::string, std::shared_ptr<model::Order>> _orders;
    std::shared_ptr<model::Quote> _quote;
    std::shared_ptr<model::Instrument> _instrument;
    std::shared_ptr<api::InstrumentApi> _instrumentApi;



    void handleQuotes(const std::vector<std::shared_ptr<model::Quote>>& quotes_, const std::string& action_);
    void handleTrades(std::vector<std::shared_ptr<model::Trade>>& trades_, const std::string& action_);
    void handlePositions(std::vector<std::shared_ptr<model::Position>>& trades_, const std::string& action_);
    void handleExecutions(std::vector<std::shared_ptr<model::Execution>>& execs_, const std::string& action_);
    void handleOrders(std::vector<std::shared_ptr<model::Order>>& orders_, const std::string& action_);

protected:
    template<typename T>
    void update(const std::vector<std::shared_ptr<T>>& data_);
    void updatePositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void insertPositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void removePositions(const std::vector<std::shared_ptr<model::Position>>& positions_);
    void insertOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);

    void removeOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);
    void updateOrders(const std::vector<std::shared_ptr<model::Order>>& orders_);

    void updateKey(const web::json::array& values_, std::vector<std::string>& key_);

public:
    ~MarketDataInterface() {};
    MarketDataInterface();
    std::shared_ptr<Event> read();
    const std::unordered_map<std::string, OrderPtr>& getOrders() const;
    const std::queue<ExecPtr>& getExecutions() const;
    const std::unordered_map<std::string, PositionPtr>& getPositions() const;
    const std::shared_ptr<model::Quote> quote() const;
    const std::shared_ptr<model::Instrument>& instrument() const;

};

using namespace io::swagger::client;

class MarketData
:   public MarketDataInterface
{
    std::string _connectionString;
    std::string _symbol;
    std::shared_ptr<ws::client::websocket_callback_client> _wsClient;

    std::string _apiSecret;
    std::string _apiKey;
    bool _initialised;
    bool _shouldAuth;

private:


    std::shared_ptr<HeartBeat> _heartBeat;
    long _cycle;

private:
    std::string getConnectionUri();
    std::string getBaseUrl();

public:
    void subscribe();

    explicit MarketData(const std::shared_ptr<Config>& config_);

    ~MarketData();

    void init();
};

#endif //TRADING_BOT_MARKETDATA_H