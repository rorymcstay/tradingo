//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "SimpleMovingAverage.h"
#include "Strategy.h"
#include "Event.h"

using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;

template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {

    using StrategyApi = Strategy<TORDApi>;

    price_t _longTermAvg;
    price_t _shortTermAvg;
    qty_t _shortExpose;
    qty_t _longExpose;

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
    // initialise base class
    StrategyApi::init(config_);

    _buyThreshold = std::atof(config_->get("buyThreshold", "0.0").c_str());
    _shortExpose = std::atof(config_->get("shortExpose", "0.0").c_str());
    _longExpose = std::atof(config_->get("longExpose", "100.0").c_str());

    auto primePercent = std::atof(config_->get("primePercent", "1.0").c_str());
    auto shortTermWindow = std::stoi(config_->get("shortTermWindow"));
    auto longTermWindow = std::stoi(config_->get("longTermWindow"));
    _smaLow = SMA_T(shortTermWindow,shortTermWindow*primePercent);
    _smaHigh = SMA_T(longTermWindow, longTermWindow*primePercent);
    LOGINFO("Breakout strategy is initialised with " << LOG_VAR(shortTermWindow) << LOG_VAR(longTermWindow) << LOG_VAR(primePercent) << LOG_NVP("buyThreshold", _buyThreshold));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {
    LOGINFO(event_->getExec()->toJson().serialize());
    LOGINFO("Position: " << StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol)->toJson().serialize());
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {
    auto trade = event_->getTrade();
    LOGINFO("onTrade: " << LOG_NVP("Price", trade->getPrice()) << LOG_NVP("Size", trade->getSize())
            << LOG_NVP("Side", trade->getSide())<< LOG_NVP("Timestamp", trade->getTimestamp().to_string()));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {
    auto quote = event_->getQuote();
    auto askPrice = quote->getAskPrice();
    auto bidPrice = quote->getBidPrice();
    auto midPoint = (askPrice+bidPrice)/2;
    _shortTermAvg = _smaLow(midPoint);
    _longTermAvg = _smaHigh(midPoint);
    LOGINFO(LOG_VAR(_shortTermAvg) << LOG_VAR(_longTermAvg));
    // TODO if signal is good if (_signal["name"]->is_good())
    if ((_smaLow.is_ready() && _smaHigh.is_ready())
         && _shortTermAvg - _longTermAvg >= _buyThreshold) {
        // short term average is higher than longterm, buy
        auto qtyToTrade = getQtyToTrade("Buy");
        LOGINFO("Signal is good " << LOG_VAR(_shortTermAvg) << LOG_VAR(_longTermAvg) << LOG_VAR(qtyToTrade));
        StrategyApi::allocations()->addAllocation(bidPrice, qtyToTrade, "Buy");
    } else if (_smaLow.is_ready() && _smaHigh.is_ready()) {
        auto qtyToTrade = getQtyToTrade("Sell");
        LOGINFO("Reverting position " << LOG_VAR(qtyToTrade));
        StrategyApi::allocations()->addAllocation(askPrice, qtyToTrade, "Sell");
    }

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::~BreakOutStrategy() {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_)
:   StrategyApi(mdPtr_, od_)
,   _shortTermAvg()
,   _longTermAvg()
,   _longExpose()
,   _shortExpose() {
}

template<typename TORDApi>
qty_t BreakOutStrategy<TORDApi>::getQtyToTrade(const std::string& side_) {
    auto allocated = StrategyApi::allocations()->totalAllocated();
    std::shared_ptr<model::Position> currentPosition = StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol);
    auto currentSize = currentPosition->getCurrentQty();
    if (side_ == "Buy") {
        // if we are buying
        return _longExpose - currentSize;

    } else {
        return _shortExpose - currentSize;
    }
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H