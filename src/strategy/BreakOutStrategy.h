//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

#include "Utils.h"
#include "api/OrderApi.h"

#include "SimpleMovingAverage.h"
#include "Signal.h"
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
    price_t _sellThreshold;

    std::string _previousDirection;
    
    double _priceAggressor = 0.01;

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
    _sellThreshold = config_->get<double>("sellThreshold", 0.0);
    _shortExpose = config_->get<double>("shortExpose", 0.0);
    _longExpose = config_->get<double>("longExpose", 1000.0);
    _displaySize = config_->get<double>("displaySize", 200);
    _priceAggressor = config_->get<double>("priceAggressor", 0.0);

    auto primePercent = config_->get<double>("primePercent", 1.0);
    auto shortTermWindow = config_->get<double>("shortTermWindow");
    auto longTermWindow = config_->get<double>("longTermWindow");

    StrategyApi::addSignal(std::make_shared<MovingAverageCrossOver>(
                StrategyApi::getMD(),
                shortTermWindow, longTermWindow));

    LOGINFO("Breakout strategy is initialised with "
            << LOG_VAR(shortTermWindow)
            << LOG_VAR(longTermWindow)
            << LOG_VAR(primePercent)
            << LOG_NVP("symbol", config_->get<std::string>("symbol"))
            << LOG_NVP("buyThreshold", _buyThreshold)
            << LOG_NVP("sellThreshold", _sellThreshold)
            );
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
    auto midPoint = (askPrice + bidPrice)/2;
    // TODO if signal is good if (_signal["name"]->is_good())
    std::string side = "";
    qty_t qtyToTrade = 0.0;
    price_t price;
    std::shared_ptr<MarketDataInterface> md = StrategyApi::getMD();

    bool isReady = StrategyApi::getSignal("moving_average_crossover")->isReady();
    Signal::Value signalValue = StrategyApi::getSignal("moving_average_crossover")->read();

    if (isReady && signalValue.direction == SignalDirection::Buy) {
        qtyToTrade = getQtyToTrade("Buy");
        price = bidPrice * (1-_priceAggressor);
        side = "Buy";
    } else if (isReady && signalValue.direction == SignalDirection::Sell) {
        qtyToTrade = getQtyToTrade("Sell");
        price = askPrice * (1+_priceAggressor);
        side = "Sell";
    } else if (isReady && signalValue.direction == SignalDirection::Hold) {
        StrategyApi::allocations()->cancelOrders([side, bidPrice, askPrice]
                (const std::shared_ptr<Allocation>& alloc_) {
                    return alloc_->getSize() != 0.0;
        });
    } else {
        LOGDEBUG("Signal is not ready.");
    }
    if (not side.empty())
        StrategyApi::allocations()->cancelOrders([side, bidPrice, askPrice]
                (const std::shared_ptr<Allocation>& alloc_) {
                    return alloc_->getSize() != 0.0 and (alloc_->getSide() != side 
                        or (std::abs(alloc_->getPrice() - (bidPrice+askPrice)/2) > 25));
        });


    if (almost_equal(qtyToTrade, 0.0)) {
        LOGDEBUG("No quantity to trade");
        return;
    }
    const std::shared_ptr<model::Position>& position = md->getPositions().at(StrategyApi::_symbol);
    if (not (
            (side == "Buy"  and std::abs(signalValue.value) >= (midPoint * _buyThreshold))
         or (side == "Sell" and std::abs(signalValue.value) >= (midPoint * _sellThreshold))
        )) {
        auto currentBalance = md->getMargin()->getWalletBalance();
        auto currentQty = position->getCurrentQty();
        if (currentBalance <= func::get_additional_cost(position, qtyToTrade, price)) {
            LOGINFO("Insufficient balance to trade "
                        << LOG_NVP("required", func::get_additional_cost(position, qtyToTrade, price))
                        << LOG_VAR(currentBalance) << LOG_VAR(currentQty) << LOG_VAR(qtyToTrade) << LOG_VAR(price));
            return;
        };
        LOGDEBUG("Signal is good, took position "
                << LOG_VAR(qtyToTrade)
                << LOG_VAR(price)
                << LOG_VAR(currentQty)
                << LOG_VAR(currentBalance));
        StrategyApi::allocations()->addAllocation(price, qtyToTrade);
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
,   _previousDirection("")
,   _priceAggressor(0.0) {

}

template<typename TOrdApi, typename TPositionApi>
qty_t BreakOutStrategy<TOrdApi, TPositionApi>::getQtyToTrade(const std::string& side_) {
    auto& md = StrategyApi::getMD();
    auto& instrument = md->getInstruments().at(StrategyApi::_symbol);
    auto& position = md->getPositions().at(StrategyApi::_symbol);
    std::shared_ptr<model::Position> currentPosition = StrategyApi::getMD()->getPositions().at(StrategyApi::_symbol);
    auto currentSize = currentPosition->getCurrentQty();
    auto target_margin = func::get_cost(instrument->getMarkPrice(),
                                        currentSize + 100.0,
                                        position->getLeverage());
    if (std::abs(StrategyApi::allocations()->totalAllocated()) > _displaySize) {
        return 0;
    }
    if (target_margin > md->getMargin()->getWalletBalance()) {
        LOGWARN("Insufficient balance to trade target delta of 100"
                << LOG_VAR(currentSize)
                << LOG_VAR(target_margin)
                << LOG_NVP("walletBalance", md->getMargin()->getWalletBalance()));
        return 0;
    };
    if (side_ == "Buy") {
        // if we are buying
        if (greater_equal(_longExpose, currentSize)) {
            return 100;
        } else {
            return 0;
        }
    } else {
        if (less_equal(-_shortExpose, currentSize)) {
            return -100;
        } else {
            return 0;
        }
    }
}


#endif //TRADINGO_BREAKOUTSTRATEGY_H
