//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "SimpleMovingAverage.h"
#include "Strategy.h"


template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {

    SimpleMovingAverage<10> _smaLow;
    SimpleMovingAverage<100> _smaHigh;
    int _highVal;
    int _lowVal;

    double _startingAmount;

public:
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;
public:
    void init(const std::shared_ptr<Config>& config_) override;

};


template<typename TORDApi>
void BreakOutStrategy<TORDApi>::init(const std::shared_ptr<Config>& config_) {
    Strategy<TORDApi>::init(config_);
    config_->get("startingAmount");

}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {
    LOGINFO("BreakOutStrategy::onExecution(): " << event_->getExec()->toJson().serialize());
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {
    _highVal = _smaHigh(event_->getTrade()->getPrice());
    _lowVal = _smaLow(event_->getTrade()->getPrice());
    LOGINFO(LOG_VAR(_highVal) << LOG_VAR(_lowVal));

}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {
    auto quote = event_->getQuote();
    auto askPrice = quote->getAskPrice();
    auto bidPrice = quote->getBidPrice();
    auto midPoint = (askPrice+bidPrice)/2;
    _lowVal = _smaLow(midPoint);
    _highVal = _smaHigh(midPoint);
    if (_lowVal > _highVal) {
        // short term average is higher than longterm, trade
        //Strategy<TORDApi>::
    }

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
,   _smaLow() {

    LOGINFO("BreakOutStrategy is initialised");
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H
