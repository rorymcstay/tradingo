//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "api/OrderApi.h"

#include "SimpleMovingAverage.h"
#include "signal/MovingAverageCrossOver.h"
#include "Strategy.h"
#include "Event.h"
#include "Functional.h"
#include "Config.h"

using namespace io::swagger::client;

using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;



template<typename TOrdApi, typename TPositionApi>
class BreakOutStrategy final : public Strategy<TOrdApi, TPositionApi> {

    using StrategyApi = Strategy<TOrdApi, TPositionApi>;

    qty_t _shortExpose;
    qty_t _longExpose;
    qty_t _displaySize;

    price_t _buyThreshold;

    std::string _previousDirection;

public:
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,
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
void BreakOutStrategy<TOrdApi, TPositionApi>::init(const std::shared_ptr<Config>& config_) {
    // initialise base class
    // TODO move to constructor
    StrategyApi::init(config_);

    _buyThreshold = config_->get<double>("buyThreshold", 0.0);
    _shortExpose = config_->get<double>("shortExpose", 0.0);
    _longExpose = config_->get<double>("longExpose", 1000.0);
    _displaySize = config_->get<double>("displaySize", 200);

    auto primePercent = config_->get<double>("primePercent", 1.0);
    auto shortTermWindow = config_->get<int>("shortTermWindow");
    auto longTermWindow = config_->get<int>("longTermWindow");

    StrategyApi::addSignal(std::make_shared<MovingAverageCrossOver>(
                StrategyApi::getMD(),
                shortTermWindow, longTermWindow));

    LOGINFO("Breakout strategy is initialised with "
            << LOG_VAR(shortTermWindow)
            << LOG_VAR(longTermWindow)
            << LOG_VAR(primePercent)
            << LOG_NVP("symbol", config_->get<std::string>("symbol"))
            << LOG_NVP("buyThreshold", _buyThreshold));
}

template<typename TOrdApi, typename TPositionApi>
void BreakOutStrategy<TOrdApi, TPositionApi>::onExecution(const std::shared_ptr<Event> &event_) {
    std::shared_ptr<MarketDataInterface> md = StrategyApi::getMD();
    auto exec = event_->getExec();
    auto margin = md->getMargin();
    auto balance = margin->getWalletBalance();
    LOGINFO(LOG_NVP("ordStatus", exec->getOrdStatus()) << LOG_NVP("price", exec->getPrice())
            << LOG_NVP("orderQty", exec->getOrderQty()) << LOG_NVP("leavesQty", exec->getLeavesQty())
            << LOG_NVP("cumQty", exec->getCumQty()) << LOG_NVP("lastQty", exec->getLastQty()));
    std::shared_ptr<model::Position> pos = md->getPositions().at(StrategyApi::_symbol);
    LOGINFO("Position: " << LOG_NVP("CurrentQty", pos->getCurrentQty())
            << LOG_NVP("CurrentCost", pos->getCurrentCost())
            << LOG_NVP("UnrealisedPnl", pos->getUnrealisedPnl())
            << LOG_NVP("RealisedPnl", pos->getRealisedPnl())
            << LOG_NVP("UnrealisedRoe%", pos->getUnrealisedRoePcnt())
            << LOG_NVP("Balance", balance)
            << LOG_NVP("LiquidationPrice", pos->getLiquidationPrice()));

}

template<typename TOrdApi, typename TPositionApi>
void BreakOutStrategy<TOrdApi, TPositionApi>::onTrade(const std::shared_ptr<Event> &event_) {
    auto trade = event_->getTrade();
    LOGDEBUG(LOG_NVP("Price", trade->getPrice()) << LOG_NVP("Size", trade->getSize())
            << LOG_NVP("Side", trade->getSide())<< LOG_NVP("Timestamp", trade->getTimestamp().to_string()));
}

template<typename TOrdApi, typename TPositionApi>
void BreakOutStrategy<TOrdApi, TPositionApi>::onBBO(const std::shared_ptr<Event> &event_) {
    auto quote = event_->getQuote();
    auto askPrice = quote->getAskPrice();
    auto bidPrice = quote->getBidPrice();
    // TODO if signal is good if (_signal["name"]->is_good())
    std::string side = "Not ready";
    qty_t qtyToTrade;
    price_t price;
    std::shared_ptr<MarketDataInterface> md = StrategyApi::getMD();

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
    } else {
        LOGDEBUG("Signal is not ready.");
    }

    if (almost_equal(qtyToTrade, 0.0)) {
        LOGDEBUG("No quantity to trade");
        return;
    }
    auto alloc =std::make_shared<Allocation>(price, qtyToTrade);
    const std::shared_ptr<model::Position>& position = md->getPositions().at(StrategyApi::_symbol);
    auto additional_cost = func::get_additional_cost(position, alloc->getTargetDelta(), alloc->getPrice());
    auto currentBalance = md->getMargin()->getWalletBalance();
    if (currentBalance >= additional_cost) {
        LOGINFO(LOG_NVP("Signal", side) << LOG_VAR(signalValue)
                                        << LOG_VAR(qtyToTrade) << LOG_VAR(price));
        if (_previousDirection != side) {
            StrategyApi::allocations()->cancelOrders([this](const std::shared_ptr<Allocation>& alloc_) {
                return !alloc_->getSide().empty() && alloc_->getSide() == _previousDirection;
            });
        }
        StrategyApi::allocations()->addAllocation(alloc);
        _previousDirection = side;
    } else {
        auto currentQty = position->getCurrentQty();
        LOGDEBUG("Signal is good, but balance is insufficient. "
                << LOG_VAR(qtyToTrade)
                << LOG_VAR(price)
                << LOG_VAR(currentQty)
                << LOG_VAR(currentBalance)
                << LOG_VAR(additional_cost));
    }
}

template<typename TOrdApi, typename TPositionApi>
BreakOutStrategy<TOrdApi, TPositionApi>::~BreakOutStrategy() {

}

template<typename TOrdApi, typename TPositionApi>
BreakOutStrategy<TOrdApi, TPositionApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,
        std::shared_ptr<TOrdApi> od_,
        std::shared_ptr<TPositionApi> posApi_,
        std::shared_ptr<InstrumentService> instSvc_)
:   StrategyApi(mdPtr_, od_, posApi_, instSvc_)
,   _longExpose()
,   _shortExpose()
,   _previousDirection(""){

}

template<typename TOrdApi, typename TPositionApi>
qty_t BreakOutStrategy<TOrdApi, TPositionApi>::getQtyToTrade(const std::string& side_) {
    std::shared_ptr<model::Position> currentPosition = StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol);
    auto currentSize = currentPosition->getCurrentQty();
    if (std::abs(StrategyApi::allocations()->totalAllocated()) > _displaySize) {
        return 0;
    }
    if (side_ == "Buy") {
        // if we are buying
        if (greater_equal(_longExpose, currentSize)) {
            return 100;
        } else {
            return 0;
        }
    } else {
        if (greater_equal(currentSize, _shortExpose)) {
            return -100;
        } else {
            return 0;
        }
    }
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H
