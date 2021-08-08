//
// Created by Rory McStay on 07/08/2021.
//

#ifndef MY_PROJECT_SIMPLESTRATEGY_H
#define MY_PROJECT_SIMPLESTRATEGY_H

#include <memory>

#include "MarketData.h"
#include "Strategy.h"

template<typename TORDApi>
class SimpleStrategy final : public Strategy<TORDApi> {
    using StrategyApi = Strategy<TORDApi>;

public:
    ~SimpleStrategy();
    SimpleStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;

    qty_t getQtyToTrade(const std::string& side_);
public:

    void init(const std::shared_ptr<Config>& config_) override;
};

template<typename TORDApi>
void SimpleStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
void SimpleStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
void SimpleStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {

    if (StrategyApi::getMD()->getPositions()[StrategyApi::_symbol])
    StrategyApi::allocations()->addAllocation(event_->getQuote()->getAskPrice(), 100, "Buy");

}


#endif //MY_PROJECT_SIMPLESTRATEGY_H
