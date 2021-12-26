//
// Created by Rory McStay on 07/08/2021.
//

#ifndef MY_PROJECT_SIMPLESTRATEGY_H
#define MY_PROJECT_SIMPLESTRATEGY_H

#include <memory>

#include "MarketData.h"
#include "Strategy.h"
#include "InstrumentService.h"

template<typename TOrdApi, typename TPositionApi>
class SimpleStrategy final : public Strategy<TOrdApi, TPositionApi> {
    using StrategyApi = Strategy<TOrdApi, TPositionApi>;

public:
    ~SimpleStrategy();
    SimpleStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,
            std::shared_ptr<TOrdApi> od_,
            std::shared_ptr<TPositionApi> posApi_,
            std::shared_ptr<InstrumentService> instSvc_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;

    qty_t getQtyToTrade(const std::string& side_);
public:

    void init(const std::shared_ptr<Config>& config_) override;
};


template<typename TOrdApi, typename TPositionApi>
SimpleStrategy<TOrdApi, TPositionApi>::SimpleStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,
            std::shared_ptr<TOrdApi> od_,
            std::shared_ptr<TPositionApi> posApi_,
            std::shared_ptr<InstrumentService> instSvc_)
{
}

template<typename TOrdApi, typename TPositionApi>
void SimpleStrategy<TOrdApi, TPositionApi>::onExecution(const std::shared_ptr<Event> &event_) {

}

template<typename TOrdApi, typename TPositionApi>
void SimpleStrategy<TOrdApi, TPositionApi>::onTrade(const std::shared_ptr<Event> &event_) {

}

template<typename TOrdApi, typename TPositionApi>
void SimpleStrategy<TOrdApi, TPositionApi>::onBBO(const std::shared_ptr<Event> &event_) {

    if (StrategyApi::getMD()->getPositions()[StrategyApi::_symbol])
    StrategyApi::allocations()->addAllocation(event_->getQuote()->getAskPrice(), 100, "Buy");

}


#endif //MY_PROJECT_SIMPLESTRATEGY_H
