//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_STRATEGY_H
#define TRADINGO_STRATEGY_H

#include <model/Order.h>

#include "MarketData.h"
#include "OrderInterface.h"

template<typename TOrdApi>
class Strategy {

    using OrderPtr = std::shared_ptr<model::Order>;

    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<OrderInterface<TOrdApi>>  _orderEngine;

    std::vector<OrderPtr> _buyOrders;
    std::vector<OrderPtr> _sellOrders;
public:
    Strategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<OrderInterface<TOrdApi>> od_);
    void evaluate();
    void init(const std::string& config_);

};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<OrderInterface<TOrdApi>> od_)
        :   _marketData(std::move(md_))
        ,   _orderEngine (std::move(od_)) {

}

template<typename TOrdApi>
void Strategy<TOrdApi>::evaluate() {
    INFO("Evaluating strategy");
    auto event = _marketData->read();
    if (event->eventType() == EventType::BBO)
    {
        INFO("BBO Update " << LOG_NVP("AskPrice",event->getQuote()->getAskPrice())
                     << LOG_NVP("BidPrice",event->getQuote()->getBidPrice()));
    }
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::string& config_) {
    INFO("Initializing strategy");
}

#endif //TRADINGO_STRATEGY_H
