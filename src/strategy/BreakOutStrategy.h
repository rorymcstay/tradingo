//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "SimpleMovingAverage.h"
#include "signal/MovingAverageCrossOver.h"
#include "Strategy.h"
#include "Event.h"

using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;

template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {

    using StrategyApi = Strategy<TORDApi>;

    qty_t _shortExpose;
    qty_t _longExpose;
    qty_t _displaySize;

    price_t _buyThreshold;

    std::string _previousDirection;

public:
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_,
                     std::shared_ptr<InstrumentService> instSvc_);
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
    // TODO move to constructor
    StrategyApi::init(config_);

    _buyThreshold = std::atof(config_->get("buyThreshold", "0.0").c_str());
    _shortExpose = std::atof(config_->get("shortExpose", "0.0").c_str());
    _longExpose = std::atof(config_->get("longExpose", "1000.0").c_str());
    _displaySize = std::atof(config_->get("displaySize", "200").c_str());

    auto primePercent = std::atof(config_->get("primePercent", "1.0").c_str());
    auto shortTermWindow = std::stoi(config_->get("shortTermWindow"));
    auto longTermWindow = std::stoi(config_->get("longTermWindow"));

    StrategyApi::addSignal(std::make_shared<MovingAverageCrossOver>(shortTermWindow, longTermWindow));



    LOGINFO("Breakout strategy is initialised with " << LOG_VAR(shortTermWindow) << LOG_VAR(longTermWindow) << LOG_VAR(primePercent) << LOG_NVP("buyThreshold", _buyThreshold));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {
    auto exec = event_->getExec();
    LOGINFO(LOG_NVP("ordStatus", exec->getOrdStatus()) << LOG_NVP("price", exec->getPrice())
            << LOG_NVP("orderQty", exec->getOrderQty()) << LOG_NVP("leavesQty", exec->getLeavesQty())
            << LOG_NVP("cumQty", exec->getCumQty()) << LOG_NVP("lastQty", exec->getLastQty()));
    std::shared_ptr<model::Position> pos = StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol);
    LOGINFO("Position: " << LOG_NVP("CurrentQty", pos->getCurrentQty())
            << LOG_NVP("CurrentCost", pos->getCurrentCost())
            << LOG_NVP("UnrealisedPnl", pos->getUnrealisedPnl())
            << LOG_NVP("RealisedPnl", pos->getRealisedPnl())
            << LOG_NVP("UnrealisedRoe%", pos->getUnrealisedRoePcnt())
            << LOG_NVP("Balance", StrategyApi::_balance));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {
    auto trade = event_->getTrade();
    LOGDEBUG(LOG_NVP("Price", trade->getPrice()) << LOG_NVP("Size", trade->getSize())
            << LOG_NVP("Side", trade->getSide())<< LOG_NVP("Timestamp", trade->getTimestamp().to_string()));
}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {
    auto quote = event_->getQuote();
    auto askPrice = quote->getAskPrice();
    auto bidPrice = quote->getBidPrice();
    // TODO if signal is good if (_signal["name"]->is_good())
    std::string side = "Not ready";
    qty_t qtyToTrade;
    price_t price;

    bool isReady = StrategyApi::getSignal("moving_average_crossover")->isReady();
    auto signalValue = isReady ? StrategyApi::getSignal("moving_average_crossover")->read() : -1 ;
    if (isReady && signalValue > 0) {  // _shortTermAvg - _longTermAvg > _buyThreshold
        // short term average is higher than longterm, buy
        qtyToTrade = getQtyToTrade("Buy");
        price = bidPrice;
        side = "Buy";
    } else if (isReady) {
        qtyToTrade = getQtyToTrade("Sell");
        price = askPrice;
        side = "Sell";
    }
    else {
        LOGDEBUG("Signal is not ready.");
    }
    if (almost_equal(qtyToTrade, 0.0)) {
        LOGDEBUG("No quantity to trade");
        return;
    } else {
        //qtyToTrade *= 0.10;
        //qtyToTrade = std::max(100.0, qtyToTrade);
        LOGINFO(LOG_NVP("Signal", side) << LOG_VAR(signalValue)
                                        << LOG_VAR(qtyToTrade) << LOG_VAR(price));
        if (_previousDirection != side) {
            StrategyApi::allocations()->cancelOrders([this](const std::shared_ptr<Allocation>& alloc_) {
                return !alloc_->getSide().empty() && alloc_->getSide() == _previousDirection;
            });
        }
        StrategyApi::allocations()->addAllocation(price, qtyToTrade, side);
        _previousDirection = side;
    }
}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::~BreakOutStrategy() {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_,
                                            std::shared_ptr<InstrumentService> instSvc_)
:   StrategyApi(mdPtr_, od_, instSvc_)
,   _longExpose()
,   _shortExpose()
,   _previousDirection(""){

}

template<typename TORDApi>
qty_t BreakOutStrategy<TORDApi>::getQtyToTrade(const std::string& side_) {
    std::shared_ptr<model::Position> currentPosition = StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol);
    auto currentSize = currentPosition->getCurrentQty();
    if (std::abs(StrategyApi::allocations()->totalAllocated()) > _displaySize) {
        return 0;
    }
    auto currentlyAllocated = StrategyApi::allocations()->totalAllocated();
    if (side_ == "Buy") {
        // if we are buying
        if (greater_equal(_longExpose, currentSize)) {
            return 100;
        } else {
            return 0;
        }
        auto toMakeUp = _longExpose - currentSize;
        if (toMakeUp < 0) {
            //
            return 0;
        } else if (greater_equal(currentlyAllocated, toMakeUp)) {
            return 0;
        } else {
            return toMakeUp;
        }
    } else {
        if (greater_equal(currentSize, _shortExpose)) {
            return -100;
        } else {
            return 0;
        }
        auto toMakeUp = (_shortExpose - currentSize);
        if (toMakeUp > 0) {
            return 0;
        } else if (less_equal(currentlyAllocated, toMakeUp)) {
            return 0;
        } else {
            return toMakeUp;
        }
    }
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H