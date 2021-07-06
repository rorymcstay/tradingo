//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_STRATEGY_H
#define TRADINGO_STRATEGY_H

#include <model/Order.h>

#include "MarketData.h"
#include "OrderInterface.h"
#include "Config.h"
#include "Utils.h"

using price_t = double;
using qty_t = int;

template<typename TOrdApi>
class Strategy {

    using OrderPtr = std::shared_ptr<model::Order>;

    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<TOrdApi>  _orderEngine;

    std::vector<OrderPtr> _buyOrders;
    std::vector<OrderPtr> _sellOrders;

    size_t _inventory = 1000;
    size_t _allocatedAsk = 0;
    size_t _allocatedBid = 0;

    price_t _ask;
    price_t _bid;

    std::shared_ptr<model::Order> _sellOrder;
    std::shared_ptr<model::Order> _buyOrder;
    int _attempted;

    std::string _symbol;
    std::string _clOrdIdPrefix;
    int _oidSeed;
    std::shared_ptr<Config> _config;

    virtual void onExecution(const std::shared_ptr<Event>& event_) = 0;
    virtual void onTrade(const std::shared_ptr<Event>& event_) = 0;
    virtual void onBBO(const std::shared_ptr<Event>& event_) = 0;

public:
    Strategy(std::shared_ptr<MarketDataInterface> md_,  std::shared_ptr<TOrdApi> od_);
    void evaluate();

    void init(const std::string& config_);
    virtual void init(const std::shared_ptr<Config>& config_);

    bool shouldEval();
    bool createOrders(price_t bid, price_t ask);;
};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<TOrdApi> od_)
:   _marketData(std::move(md_))
,   _orderEngine(std::move(od_)) {

}


template<typename TOrdApi>
void Strategy<TOrdApi>::evaluate() {
    auto event = _marketData->read();
    if (!event) {
        return;
    }
    // call one of three handlers.
    if (event->eventType() == EventType::BBO) {
        auto quote = event->getQuote();
        _bid = quote->getBidPrice();
        _ask = quote->getAskPrice();
        onBBO(event);
        DEBUG("BBO Update Bid=" << quote->getBidSize() << '@' << quote->getBidPrice()
                    << " Ask=" << quote->getAskSize() << '@' << quote->getAskPrice());
    } else if (event->eventType() == EventType::TradeUpdate) {
        DEBUG( "Trade: " << event->getTrade()->toJson().serialize());
        onTrade(event);
    } else if (event->eventType() == EventType::Exec) {
        DEBUG("Execution: " << event->getExec()->toJson().serialize());
        onExecution(event);
    }
    if (_allocatedAsk > 0 || _allocatedBid > 0) {
        //
    } else {
        createOrders(_bid, _ask);
    }
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::string& config_) {
    auto _config = std::make_shared<Config>(config_);
    init(_config);
    INFO("Initializing strategy");
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::shared_ptr<Config>& config_) {
    _config = config_;
    _symbol = _config->get("symbol");
    _clOrdIdPrefix = _config->get("clOrdPrefix");
    INFO("Initializing strategy");
    for (auto& order : _marketData->getOrders())
        INFO("Open Order: " LOG_NVP("OID", order.first) << order.second->toJson().serialize());
}

template<typename TOrdApi>
bool Strategy<TOrdApi>::shouldEval() {
    return true;
}

template<typename TOrdApi>
bool Strategy<TOrdApi>::createOrders(price_t bid, price_t ask) {
    if (_attempted) {
        return false;
    }
    auto size = _inventory / 2;
    auto buy = std::make_shared<model::Order>();
    buy->setPrice(bid);
    buy->setSide("Buy");
    buy->setOrderQty(std::round(size - _allocatedBid));
    buy->setSymbol(_symbol);
    buy->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed++));
    buy->setSymbol(_symbol);

    auto sell = std::make_shared<model::Order>();
    sell->setPrice(ask);
    sell->setSide("Sell");
    sell->setOrderQty(std::round(size - _allocatedAsk));
    sell->setSymbol(_symbol);
    sell->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed++));
    sell->setSymbol(_symbol);

    pplx::task<std::vector<std::shared_ptr<model::Order>>> tsk = _orderEngine->order_newBulk(
            [buy, sell]() {
                auto orders = web::json::value::array(2);
                int count = 0;
                for (auto& ord : {buy, sell})
                    orders[count++] = ord->toJson();
                auto ordString = orders.serialize();
                return ordString;
            }());
        tsk.then(
                //
                [=, this] (const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_) {
                    DEBUG("Send order callback");
                    try {
                        INFO("Succesfully sent order: " << task_.get()[0]->toJson().serialize());
                        _attempted = true;
                    } catch (api::ApiException& apiException) {
                        auto reason = apiException.getContent();
                        INFO("APIException caught failed to send order: " << LOG_VAR(apiException.what())
                                                                          << " ExceptionContent=" << reason->rdbuf());
                        return;
                    } catch (web::http::http_exception& httpException) {
                        INFO("Failed to send order! " << LOG_VAR(httpException.what()) << LOG_VAR(httpException.error_code()));
                        return;
                    }
                    _allocatedBid += size;
                    _buyOrder = buy;
                    _allocatedAsk += size;
                    _sellOrder = sell;
                });


    return true;
}

#endif //TRADINGO_STRATEGY_H
