//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "SimpleMovingAverage.h"
#include "Strategy.h"

using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;

template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {

    int _longTermAvg;
    int _shortTermAvg;
    price_t _startingAmount;
    price_t _buyThreshold;

    SMA_T _smaLow;
    SMA_T _smaHigh;

public:
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;

    qty_t getQtyToTrade(const std::string& side_);
public:

    void init(const std::shared_ptr<Config>& config_) override;
};


template<typename TORDApi>
void BreakOutStrategy<TORDApi>::init(const std::shared_ptr<Config>& config_) {
    Strategy<TORDApi>::init(config_);
    _buyThreshold = std::atof(config_->get("buyThreshold", "0.0").c_str());
    auto primePercent = std::atof(config_->get("primePercent", "0.5").c_str());
    auto shortTermWindow = std::stoi(config_->get("shortTermWindow"));
    auto longTermWindow = std::stoi(config_->get("longTermWindow"));
    _smaLow = SMA_T(shortTermWindow,shortTermWindow*primePercent);
    _smaHigh = SMA_T(longTermWindow, longTermWindow*primePercent);
    LOGINFO("Breakout strategy is initialised with " << LOG_VAR(shortTermWindow) << LOG_VAR(longTermWindow) << LOG_VAR(primePercent) << LOG_NVP("buyThreshold", _buyThreshold));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {
    LOGINFO("BreakOutStrategy::onExecution(): " << event_->getExec()->toJson().serialize());
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {
    _longTermAvg = _smaHigh(event_->getTrade()->getPrice());
    _shortTermAvg = _smaLow(event_->getTrade()->getPrice());
    LOGINFO(LOG_VAR(_longTermAvg) << LOG_VAR(_shortTermAvg));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {
    auto quote = event_->getQuote();
    auto askPrice = quote->getAskPrice();
    auto bidPrice = quote->getBidPrice();
    auto midPoint = (askPrice+bidPrice)/2;
    _shortTermAvg = _smaLow(midPoint);
    _longTermAvg = _smaHigh(midPoint);
    if ((_smaLow.is_ready() && _smaHigh.is_ready()) && _shortTermAvg - _longTermAvg >= _buyThreshold) {
        // short term average is higher than longterm, buy
        Strategy<TORDApi>::addAllocation(bidPrice, getQtyToTrade("Buy"), "Buy");
    } else {
        //
        Strategy<TORDApi>::addAllocation(askPrice, getQtyToTrade("Sell"), "Sell");
    }

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::~BreakOutStrategy() {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_)
:   Strategy<TORDApi>(mdPtr_, od_)
, _shortTermAvg()
, _longTermAvg()
, _startingAmount() {
}

template<typename TORDApi>
qty_t BreakOutStrategy<TORDApi>::getQtyToTrade(const std::string& side_) {
    qty_t buyExposure = 0.0;
    qty_t sellExposure = 0.0;
    for (auto& allocation : Strategy<TORDApi>::allocations()) {
        if (!allocation)
            continue;
        if (allocation->getSide() == "Buy") {
            buyExposure += allocation->getSize();
        } else {
            sellExposure += allocation->getSize();
        }
    }
    if (side_ == "Buy") {
        return sellExposure;
    } else {
        return buyExposure;
    }
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H