//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "SimpleMovingAverage.h"
#include "Strategy.h"


template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {
public:
    SimpleMovingAverage<10> _smaLow;
    SimpleMovingAverage<100> _smaHigh;
    int _highVal;
    int _lowVal;
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;

};

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {
    INFO("BreakOutStrategy::onExecution(): " << event_->getExec()->toJson().serialize());
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {
    _highVal = _smaHigh(event_->getTrade()->getPrice());
    _lowVal = _smaLow(event_->getTrade()->getPrice());
    INFO(LOG_VAR(_highVal) << LOG_VAR(_lowVal));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::~BreakOutStrategy() {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_)
:   Strategy<TORDApi>(mdPtr_, od_)
,   _lowVal(0)
,   _highVal(0)
,   _smaHigh()
,    _smaLow(){
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H
