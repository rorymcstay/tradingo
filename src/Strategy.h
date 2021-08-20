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

    using OrderPtr = std::shared_ptr<model::Order>;


    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<TOrdApi>  _orderEngine;


    std::string _clOrdIdPrefix;
    int _oidSeed;
    std::shared_ptr<Config> _config;

    std::shared_ptr<Allocations>           _allocations;
    std::unordered_map<long, OrderPtr> _orders;
    std::unordered_map<std::string, std::shared_ptr<Signal>> _signals;


    virtual void onExecution(const std::shared_ptr<Event>& event_) = 0;
    virtual void onTrade(const std::shared_ptr<Event>& event_) = 0;
    virtual void onBBO(const std::shared_ptr<Event>& event_) = 0;
    std::shared_ptr<model::Order> createOrder(const std::shared_ptr<Allocation>& allocation_);
    /// update orders after call to api.
    void updateFromTask(const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_);

public:
    Strategy(std::shared_ptr<MarketDataInterface> md_,  std::shared_ptr<TOrdApi> od_);
    void evaluate();
    void init(const std::string& config_);
    virtual void init(const std::shared_ptr<Config>& config_);
    virtual bool shouldEval();
    const std::shared_ptr<Allocations>& allocations() { return _allocations; }
    std::shared_ptr<MarketDataInterface> getMD() const { return _marketData; }
    void forEachSignal(std::function<void(const Signal::Map::value_type&)> function_) {
        std::for_each(_signals.begin(), _signals.end(),function_);
    }
    Signal::Ptr getSignal(const std::string& name) {
        return _signals[name];
    }


protected:
    // allocation api
    std::string _symbol;
    price_t _balance;

    void addSignal(const std::shared_ptr<Signal>& signal_) {
        _signals.emplace(signal_->name(), signal_);
        signal_->init(_config, _marketData);
    }

public:
    std::shared_ptr<model::Instrument> instrument() const { return _marketData ? _marketData->instrument() : nullptr; }

    void placeAllocations();
};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<TOrdApi> od_)
:   _marketData(std::move(md_))
,   _orderEngine(std::move(od_))
,   _allocations(nullptr)
,   _oidSeed(std::chrono::system_clock::now().time_since_epoch().count()) {

}


template<typename TOrdApi>
void Strategy<TOrdApi>::evaluate() {
    LOGDEBUG(AixLog::Color::YELLOW << "======== START Evaluate ========" << AixLog::Color::none);
    auto event = _marketData->read();
    if (!event) {
        return;
    }
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
    auto tickSize = instrument()->getTickSize();
    auto referencePrice = instrument()->getPrevPrice24h();
    auto lotSize = instrument()->getLotSize();
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
    //_amends.clear(); _newOrders.clear(); _cancels.clear();
    // TODO use ranges ontop of *_allocations iterator. - occupied price level view
    for (auto& allocation : *_allocations) {
        if (!allocation) {
            continue;
        }
        if (almost_equal(allocation->getTargetDelta(), 0.0)) {
            continue;
        }
        LOGINFO("Processing allocation " << LOG_NVP("targetDelta",allocation->getTargetDelta())
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
            //order->setOrderID(currentOrder->getOrderID());
            // 2021-07-11 22-31-13.707 [Info] (updateFromTask) 	ApiException: {"error":{"message":"You may send orderID or origClOrdID, but not both.","name":"ValidationError"}} apiException.error_code()='generic:400', apiException.what()='error calling order_amendBulk: Bad Request',  |Strategy.h:282
            order->setOrigClOrdID(currentOrder->getClOrdID());
            //assert(currentOrder->getLeavesQty() == allocation->getSize() && "Allocation + LeavesQty out of sync");
            // we have an order at this price level
            if (allocation->isChangingSide()) {
                LOGINFO("Chaging sides");
                currentOrder->setOrdStatus("PendingCancel");
                currentOrder->setOrderQty(0.0);
                /*
                auto cancel = createOrder(allocation);
                // cancel will be for opposite side
                cancel->setSide("Buy" == allocation->getSide() ? "Sell" : "Buy");
                cancel->setOrderQty(0);
                cancel->setOrderID(currentOrder->getOrderID());
                cancel->setOrigClOrdID(order->getOrigClOrdID());
                 */
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
                _orderEngine->order_cancel(boost::none,
                                           toSend->origClOrdIDIsSet() ? toSend->getOrigClOrdID() : toSend->getClOrdID(),
                                           std::string("Allocation removed")).then(
                    [this, &toSend](const pplx::task<std::vector<std::shared_ptr<model::Order>>> &orders_) {
                        this->updateFromTask(orders_);
                    });
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
            _orderEngine->order_amendBulk(jsList.serialize()).then(
                [this, &_amends](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                    this->updateFromTask(orders_);
                });
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
            _orderEngine->order_newBulk(jsList.serialize()).then(
                [this, &_newOrders](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                        this->updateFromTask(orders_);
            });
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


#endif //TRADINGO_STRATEGY_H
