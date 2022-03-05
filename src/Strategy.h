//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_STRATEGY_H
#define TRADINGO_STRATEGY_H

#include <model/Order.h>
#include <model/Instrument.h>
#include <stdexcept>

#include "MarketData.h"
#include "OrderInterface.h"
#include "Config.h"
#include "Utils.h"
#include "Allocation.h"
#include "Signal.h"
#include "Allocations.h"
#include "CallbackTimer.h"
#include "api/PositionApi.h"


template<typename TOrdApi,
    typename TPositionApi>
class Strategy {
    /*
     * Base strategy class for trading strategies.
     * The strategy api consists of three handler methods
     * and an Allocations set. This enables the strategy
     * to place orders onto market.
     */

    using OrderPtr = std::shared_ptr<model::Order>;
    using TAllocations =Allocations<TOrdApi>;

    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<TOrdApi>             _orderEngine;
    std::shared_ptr<TPositionApi>        _positionApi;

    std::string                          _clOrdIdPrefix;
    std::shared_ptr<Config>              _config;

    std::shared_ptr<TAllocations>        _allocations;

    std::unordered_map<std::string, std::shared_ptr<Signal>> _timed_signals;
    std::unordered_map<std::string, std::shared_ptr<Signal>> _callback_signals;

    std::shared_ptr<model::Instrument> _instrument;
    std::shared_ptr<InstrumentService> _instrumentService;
    /// own execution handler method
    virtual void onExecution(const std::shared_ptr<Event>& event_) = 0;
    /// trade in market handler method
    virtual void onTrade(const std::shared_ptr<Event>& event_) = 0;
    /// bbo update hanlder method
    virtual void onBBO(const std::shared_ptr<Event>& event_) = 0;

public:
    Strategy(std::shared_ptr<MarketDataInterface> md_,
        std::shared_ptr<TOrdApi> od_,
        std::shared_ptr<TPositionApi> posApi_,
        std::shared_ptr<InstrumentService> instService_);
    /// read an event of event queue and call handler.
    void evaluate();

    /// read config, get instrument and create allocations
    virtual void init(const std::shared_ptr<Config>& config_);
    /// if we should continue trading.
    virtual bool shouldEval();
    /// the strategies allocations, in the future, might be need for > 1 set of allocations. i.e multi instrument
    /// strategies
    const std::shared_ptr<TAllocations>& allocations() { return _allocations; }
    /// market data accessor
    std::shared_ptr<MarketDataInterface> getMD() const { return _marketData; }
    /// conduct function_ on each signal. Optionally only evaluate on callback types
    /// update siganls on the current event
    void updateSignals();
    /// get a signal
    Signal::Ptr getSignal(const std::string& name);

    void setLeverage(double leverage_);

private:
    void forEachSignal(std::function<void(const Signal::Map::value_type&)> function_, bool callbacks=true) {
        if (callbacks) {
            std::for_each(_callback_signals.begin(), _callback_signals.end(), function_);
        } else {
            std::for_each(_timed_signals.begin(), _timed_signals.end(), function_);
        }
    }


protected:
    // allocation api
    std::string _symbol;
    /// add a signal to strategy
    void addSignal(const std::shared_ptr<Signal>& signal_);

public:
    /// instrument accessor
    std::shared_ptr<model::Instrument> instrument() const { return _instrument; }
};

template<typename TOrdApi, typename TPositionApi>
Strategy<TOrdApi, TPositionApi>::Strategy(std::shared_ptr<MarketDataInterface> md_,
        std::shared_ptr<TOrdApi> od_,
        std::shared_ptr<TPositionApi> posApi_,
        std::shared_ptr<InstrumentService> instrumentSvc_)
:   _marketData(std::move(md_))
,   _orderEngine(std::move(od_))
,   _positionApi(std::move(posApi_))
,   _allocations(nullptr)
,   _instrumentService(std::move(instrumentSvc_))
{

}


template<typename TOrdApi, typename TPositionApi>
void Strategy<TOrdApi, TPositionApi>::setLeverage(double leverage_) {
    _positionApi->position_updateLeverage(_symbol, leverage_);
}


template<typename TOrdApi, typename TPositionApi>
void Strategy<TOrdApi, TPositionApi>::evaluate() {
    LOGDEBUG(AixLog::Color::YELLOW << "======== START Evaluate ========" << AixLog::Color::none);
    auto event = _marketData->read();
    if (!event) {
        return;
    }
    updateSignals();
    // call one of three handlers.
    if (event->eventType() == EventType::BBO) {
        onBBO(event);
    } else if (event->eventType() == EventType::TradeUpdate) {
        onTrade(event);
    } else if (event->eventType() == EventType::Exec) {
        auto exec = event->getExec();
        _allocations->update(exec);
        onExecution(event);
    } else {
        throw std::runtime_error("unhandled event type");
    }
    if (allocations()->modified()) {
        _allocations->placeAllocations();
    }
    LOGDEBUG(AixLog::Color::YELLOW << "======== FINISH Evaluate ========" << AixLog::Color::none);
}

template<typename TOrdApi, typename TPositionApi>
void Strategy<TOrdApi, TPositionApi>::init(const std::shared_ptr<Config>& config_) {
    LOGINFO("Initializing strategy");
    _config = config_;

    _symbol = _config->get<std::string>("symbol");

    auto& instrument = _marketData->getInstruments().at(_symbol);
    auto tickSize = instrument->getTickSize();
    auto referencePrice = instrument->getPrevPrice24h();
    auto lotSize = instrument->getLotSize();

    LOGINFO("Initialising allocations with " << LOG_VAR(referencePrice) << LOG_VAR(tickSize));
    int cloidSeed = std::chrono::system_clock::now().time_since_epoch().count();
    if (not (_config->get<int>("cloidSeed", -1) == -1)) {
        cloidSeed = _config->get<int>("cloidSeed");
    }
    _clOrdIdPrefix = _config->get<std::string>("clOrdPrefix");
    _allocations = std::make_shared<TAllocations>(_orderEngine,
                                                  _symbol,
                                                  _clOrdIdPrefix,
                                                  cloidSeed,
                                                  referencePrice,
                                                  tickSize, lotSize);
 
    double initialLeverage = _config->get<double>("initialLeverage", 10.0);
    setLeverage(initialLeverage);
}

template<typename TOrdApi, typename TPositionApi>
bool Strategy<TOrdApi, TPositionApi>::shouldEval() {
    return true;
}


template<typename TOrdApi, typename TPositionApi>
void Strategy<TOrdApi, TPositionApi>::updateSignals() {
    forEachSignal([](const Signal::Map::value_type& signal_) { signal_.second->update(); });
}

template<typename TOrdApi, typename TPositionApi>
void Strategy<TOrdApi, TPositionApi>::addSignal(const std::shared_ptr<Signal> &signal_) {
    signal_->init(_config, _marketData);
    // signals are either globally callback to disable timer thread during tests.
    if (signal_->callback() or _config->get<bool>("override-signal-callback")) {
        _callback_signals.emplace(signal_->name(), signal_);
    } else {
        _timed_signals.emplace(signal_->name(), signal_);
    }
}

template<typename TOrdApi, typename TPositionApi>
Signal::Ptr Strategy<TOrdApi, TPositionApi>::getSignal(const std::string &name) {
    if (_timed_signals.find(name) != _timed_signals.end()) {
        return _timed_signals[name];
    }
    if (_callback_signals.find(name) != _callback_signals.end()) {
        return _callback_signals[name];
    }
    return nullptr;
}


#endif //TRADINGO_STRATEGY_H
