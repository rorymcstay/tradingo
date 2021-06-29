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

    size_t _inventory;
    size_t _allocatedAsk = 0;
    size_t _allocatedBid = 0;

    price_t _ask;
    price_t _bid;

    std::shared_ptr<model::Order> _sellOrder;
    std::shared_ptr<model::Order> _buyOrder;

    std::string _symbol;
    std::string _clOrdIdPrefix;
    int _oidSeed;
public:
    Strategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TOrdApi> od_);
    void evaluate();
    void init(const std::string& config_);

    void init(const std::shared_ptr<Config> &config_);
    bool shouldEval();

    bool createOrders(price_t bid, price_t ask);;
};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<TOrdApi> od_)
:   _marketData(std::move(md_))
,   _orderEngine (std::move(od_)) {

}

template<typename TOrdApi>
void Strategy<TOrdApi>::evaluate() {
    auto event = _marketData->read();
    if (!event) {
        return;
    }
    if (event->eventType() == EventType::BBO) {
        auto quote = event->getQuote();
        _bid = quote->getBidPrice();
        _ask = quote->getAskPrice();
        INFO("BBO Update Bid=" << quote->getBidSize() << '@' << quote->getBidPrice()
                    << " Ask=" << quote->getAskSize() << '@' << quote->getAskPrice());
    }
    if (event->eventType() == EventType::TradeUpdate) {
        INFO( "Trade: " << event->getTrade()->toJson().serialize());
    }
    if (_allocatedAsk > 0 || _allocatedBid > 0) {
        //
    } else {
        createOrders(_bid, _ask);
    }
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::string& config_) {
    INFO("Initializing strategy");
}


template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::shared_ptr<Config>& config_) {
    INFO("Initializing strategy");
}

template<typename TOrdApi>
bool Strategy<TOrdApi>::shouldEval() {
    return true;
}

template<typename TOrdApi>
bool Strategy<TOrdApi>::createOrders(price_t bid, price_t ask) {
    auto size = _inventory / 2;
    auto buy = std::make_shared<model::Order>();
    buy->setPrice(bid);
    buy->setSide("Buy");
    buy->setOrderQty(size);
    buy->setSymbol(_symbol);
    buy->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed));
    _allocatedBid += size;
    _buyOrder = buy;


    auto sell = std::make_shared<model::Order>();
    sell->setPrice(ask);
    sell->setSide("Sell");
    sell->setOrderQty(size - _allocatedAsk);
    sell->setSymbol(_symbol);
    sell->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed));
    _allocatedAsk += size;
    _sellOrder = sell;

    _orderEngine->order_newBulk(
            [buy, sell]() {
                thread_local std::vector<std::string> orders;
                orders.reserve(2);
                orders.clear();
                for (auto& ord : {buy, sell})
                    orders.push_back(ord->toJson().serialize());
            }());
    );
}

#endif //TRADINGO_STRATEGY_H
