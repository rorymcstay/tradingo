//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_STRATEGY_H
#define TRADINGO_STRATEGY_H

#include <model/Order.h>
#include <model/Instrument.h>

#include "MarketData.h"
#include "OrderInterface.h"
#include "Config.h"
#include "Utils.h"
#include "Allocation.h"
#include "Signal.h"
#include "Allocations.h"
#include "CallbackTimer.h"


template<typename TOrdApi>
class Strategy {
    /*
     * Base strategy class for trading strategies.
     * The strategy api consists of three handler methods
     * and an Allocations set. This enables the strategy
     * to place orders onto market.
     */

    using OrderPtr = std::shared_ptr<model::Order>;

    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<TOrdApi>             _orderEngine;

    std::string                          _clOrdIdPrefix;
    int                                  _oidSeed;
    std::shared_ptr<Config>              _config;

    std::shared_ptr<Allocations>         _allocations;
    std::unordered_map<long, OrderPtr>   _orders;

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
    /// create a new order from an allocation
    std::shared_ptr<model::Order> createOrder(const std::shared_ptr<Allocation>& allocation_);
    /// update orders after call to api.
    void updateFromTask(const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_);

public:
    Strategy(std::shared_ptr<MarketDataInterface> md_,  std::shared_ptr<TOrdApi> od_,
             std::shared_ptr<InstrumentService> instService_);
    /// read an event of event queue and call handler.
    void evaluate();

    /// read config, get instrument and create allocations
    virtual void init(const std::shared_ptr<Config>& config_);
    /// convenience method on the above, instead read config_ from file.
    void init(const std::string& config_);
    /// if evaluate should be called.
    virtual bool shouldEval();
    /// the strategies allocations, in the future, might be need for > 1 set of allocations. i.e multi instrument
    /// strategies
    const std::shared_ptr<Allocations>& allocations() { return _allocations; }
    /// market data accessor
    std::shared_ptr<MarketDataInterface> getMD() const { return _marketData; }
    /// conduct function_ on each signal. Optionally only evaluate on callback types
    /// update siganls on the current event
    void updateSignals();
    /// get a signal
    Signal::Ptr getSignal(const std::string& name);

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
    price_t _balance{};
    /// add a signal to strategy
    void addSignal(const std::shared_ptr<Signal>& signal_);

public:
    /// instrument accessor
    std::shared_ptr<model::Instrument> instrument() const { return _instrument; }
    /// reflect current allocations onto exchange
    void placeAllocations();
};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<TOrdApi> od_,
                            std::shared_ptr<InstrumentService> instrumentSvc_)
:   _marketData(std::move(md_))
,   _orderEngine(std::move(od_))
,   _allocations(nullptr)
,   _instrumentService(std::move(instrumentSvc_))
,   _oidSeed(std::chrono::system_clock::now().time_since_epoch().count()) {

}


template<typename TOrdApi>
void Strategy<TOrdApi>::evaluate() {
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
        _allocations->update(event->getExec());
        auto exec = event->getExec();
        if (exec->getExecType() == "Trade") {
            // TODO calculate/update balance private method.
            _balance += (1/ exec->getLastPx() * exec->getLastQty() * ((exec->getSide() == "Buy") ? -1 : 1));
        }
        onExecution(event);
    }

    if (allocations()->modified()) {
        placeAllocations();
    }
    LOGDEBUG(AixLog::Color::YELLOW << "======== FINISH Evaluate ========" << AixLog::Color::none);
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::string& config_) {
    init(config_);
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::shared_ptr<Config>& config_) {
    LOGINFO("Initializing strategy");
    _config = config_;

    _symbol = _config->get("symbol");
    _clOrdIdPrefix = _config->get("clOrdPrefix");

    auto cloidSeed = _config->get("cloidSeed", "");
    if (!cloidSeed.empty()) {
        _oidSeed = std::stoi(cloidSeed);
    }
    auto instrument = _instrumentService->get(_symbol);
    auto tickSize = instrument.getTickSize();
    auto referencePrice = instrument.getPrevPrice24h();
    auto lotSize = instrument.getLotSize();
    _balance = std::atof(_config->get("balance", "0.01").c_str());

    LOGINFO("Initialising allocations with " << LOG_VAR(referencePrice) << LOG_VAR(tickSize));
    _allocations = std::make_shared<Allocations>(referencePrice, tickSize, lotSize);

}

template<typename TOrdApi>
bool Strategy<TOrdApi>::shouldEval() {
    return true;
}


template<typename TOrdApi>
void Strategy<TOrdApi>::placeAllocations() {
    std::vector<std::shared_ptr<model::Order>> _amends;
    std::vector<std::shared_ptr<model::Order>> _newOrders;
    std::vector<std::shared_ptr<model::Order>> _cancels;
    _amends.clear(); _newOrders.clear(); _cancels.clear();
    // TODO use ranges ontop of *_allocations iterator. - occupied price level view
    for (auto& allocation : *_allocations) {
        if (!allocation) {
            continue;
        }
        if (almost_equal(allocation->getTargetDelta(), 0.0)) {
            continue;
        }
        LOGDEBUG("Processing allocation " << LOG_NVP("targetDelta",allocation->getTargetDelta())
                << LOG_NVP("currentSize", allocation->getSize()) << LOG_NVP("price",allocation->getPrice()));

        std::shared_ptr<model::Order> order = createOrder(allocation);
        size_t priceIndex = _allocations->allocIndex(order->getPrice());
        auto& currentOrder = _orders[priceIndex];
        if (currentOrder) {
            LOGINFO("Price level is occupied " << LOG_VAR(order->getPrice())
                    << LOG_VAR(order->getOrderQty())
                    << LOG_VAR(currentOrder->getOrderQty())
                    << LOG_VAR(currentOrder->getPrice())
                    << LOG_VAR(currentOrder->getOrderID()));
            // ApiException: {"error":{"message":"You may send orderID or origClOrdID, but not both.","name":"ValidationError"}}
            order->setOrigClOrdID(currentOrder->getClOrdID());
            //assert(currentOrder->getLeavesQty() == allocation->getSize() && "Allocation + LeavesQty out of sync");
            // we have an order at this price level
            if (allocation->isChangingSide()) {
                LOGINFO("Chaging sides");
                currentOrder->setOrdStatus("PendingCancel");
                currentOrder->setOrderQty(0.0);
                _cancels.push_back(currentOrder);
                order->setOrderID("");
                order->unsetOrigClOrdID();
                _newOrders.push_back(order);

            } else if (allocation->isAmendDown()) {
                LOGINFO("Amending down: " << LOG_VAR(order->getClOrdID()) << LOG_VAR(order->getPrice())
                                          << LOG_VAR(order->getOrderQty()));
                order->setOrdStatus("PendingAmend");
                _amends.push_back(order);
            } else if (allocation->isAmendUp()) {
                LOGINFO("Amending up: " << LOG_VAR(order->getClOrdID()) << LOG_VAR(order->getPrice())
                                        << LOG_VAR(order->getOrderQty()));
                order->setOrdStatus("PendingAmend");
                _amends.push_back(order);
            } else if (allocation->isCancel()) {
                LOGINFO("Cancelling: " << LOG_VAR(order->getClOrdID()) << LOG_VAR(order->getPrice())
                                       << LOG_VAR(order->getOrderQty()));
                currentOrder->setOrdStatus("PendingCancel");
                currentOrder->setOrderQty(0);
                _cancels.push_back(order);
            } else if (allocation->isNew()) {
                LOGWARN("Order present but new allocation.");
                _newOrders.push_back(order);
            } else {
                LOGWARN("Couldn't determine action");
            }
        } else if (allocation->isNew()) {
            _newOrders.push_back(order);
            LOGINFO("Placing new allocation: " << LOG_VAR(order->getClOrdID()) << LOG_VAR(order->getPrice())
                        << LOG_VAR(order->getOrderQty()));
        } else {
            LOGWARN("Missing order: " << LOG_NVP("Price", allocation->getPrice())
                << LOG_NVP("Size", allocation->getSize())
                << LOG_NVP("TargetDelta", allocation->getTargetDelta()));
            _newOrders.push_back(order);
        }
    }

    // make cancels first
    // TODO cannot do error handling for changingSide - why not cancel or reset allocation on ack.
    // TODO placeCancels method.
    for (auto& toSend : _cancels) {
        if (!_cancels.empty()) {
            try {
                auto task = _orderEngine->order_cancel(boost::none,
                                           toSend->origClOrdIDIsSet() ? toSend->getOrigClOrdID() : toSend->getClOrdID(),
                                           std::string("")).then(
                    [this, &toSend](const pplx::task<std::vector<std::shared_ptr<model::Order>>> &orders_) {
                        this->updateFromTask(orders_);
                    });
                task.wait();

            } catch (api::ApiException &ex_) {
                LOGERROR("Error cancelling order " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what())
                                                   << LOG_NVP("order", toSend->toJson().serialize()));
                _allocations->get(toSend->getPrice())->cancelDelta();
            }

        }
    }

    // TODO placeOrderAmends method
    auto jsList = web::json::value::array();
    int count = 0;
    for (auto& toSend : _amends) {
        jsList.as_array()[count++] = toSend->toJson();
    }
    // and then amends
    if (!_amends.empty()) {
        try {
            auto task = _orderEngine->order_amendBulk(jsList.serialize()).then(
                [this, &_amends](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                    this->updateFromTask(orders_);
                });
            task.wait();
        } catch (api::ApiException &ex_) {
            LOGERROR("Error amending orders: " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what()));
            _allocations->cancel([](const std::shared_ptr<Allocation> &alloc_) {
                return alloc_->isAmendDown() || alloc_->isAmendUp();
            });
        }
    }

    // TODO: placeNewOrders method.
    jsList = web::json::value::array();
    count = 0;
    for (auto& toSend : _newOrders) {
        jsList.as_array()[count++] = toSend->toJson();
    }
    if (!_newOrders.empty()) {
        try {
            auto task = _orderEngine->order_newBulk(jsList.serialize()).then(
                [this, &_newOrders](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                        this->updateFromTask(orders_);
            });
            task.wait();
        } catch (api::ApiException &ex_) {
            LOGERROR("Error placing new orders " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what()));
            _allocations->cancel([](const std::shared_ptr<Allocation>& alloc_) { return alloc_->isNew() || alloc_->isChangingSide(); });
        }
    }

    LOGINFO("Allocations have been reflected. " << LOG_NVP("amend", _amends.size())
            << LOG_NVP("new", _newOrders.size()) << LOG_NVP("cancel", _cancels.size()));
    _allocations->restAll();
    _cancels.clear();
    _newOrders.clear();
    _amends.clear();

}

template<typename TOrdApi>
std::shared_ptr<model::Order> Strategy<TOrdApi>::createOrder(const std::shared_ptr<Allocation> &allocation_) {
    auto newOrder = std::make_shared<model::Order>();
    newOrder->setPrice(allocation_->getPrice());
    newOrder->setSide(allocation_->targetSide());
    newOrder->setOrderQty(std::abs(allocation_->getSize()+allocation_->getTargetDelta()));
    newOrder->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed++));
    newOrder->setSymbol(_symbol);
    return newOrder;
}

template<typename TOrdApi>
void Strategy<TOrdApi>::updateFromTask(const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_) {
    for (auto& order : task_.get()) {
        long index = _allocations->allocIndex(order->getPrice());
        LOGINFO("Success: " << LOG_NVP("price", order->getPrice())
                << LOG_NVP("ordStatus", order->getOrdStatus())
                << LOG_NVP("orderQty", order->getOrderQty())
                << LOG_NVP("leavesQty",order->getLeavesQty())
                << LOG_NVP("cumQty", order->getCumQty()));
        _orders[index] = order;

        if (order->getOrdStatus() == "Canceled") {
            long priceIndex = _allocations->allocIndex(order->getPrice());
            _orders.erase(priceIndex);
        }
    }
}

template<typename TOrdApi>
void Strategy<TOrdApi>::updateSignals() {
    forEachSignal([](const Signal::Map::value_type& signal_) { signal_.second->update(); });
}

template<typename TOrdApi>
void Strategy<TOrdApi>::addSignal(const std::shared_ptr<Signal> &signal_) {
    signal_->init(_config, _marketData);
    // signals are either globally callback to disable timer thread during tests.
    if (signal_->callback() or _config->get("override-signal-callback", "false") == "true") {
        _callback_signals.emplace(signal_->name(), signal_);
    } else {
        _timed_signals.emplace(signal_->name(), signal_);
    }
}

template<typename TOrdApi>
Signal::Ptr Strategy<TOrdApi>::getSignal(const std::string &name) {
    if (_timed_signals.find(name) != _timed_signals.end()) {
        return _timed_signals[name];
    }
    if (_callback_signals.find(name) != _callback_signals.end()) {
        return _callback_signals[name];
    }
    return nullptr;
}


#endif //TRADINGO_STRATEGY_H
