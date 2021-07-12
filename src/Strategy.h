//
// Created by Rory McStay on 19/06/2021.
//

#ifndef TRADINGO_STRATEGY_H
#define TRADINGO_STRATEGY_H

#include <model/Order.h>

#include "MarketData.h"
#include "OrderInterface.h"
#include "Config.h"
#include "Utils.h"
#include "Allocation.h"
#include "model/Instrument.h"
#include "Allocations.h"


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

protected:
    // allocation api
    std::string _symbol;

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
    auto event = _marketData->read();
    if (!event) {
        return;
    }
    // call one of three handlers.
    if (event->eventType() == EventType::BBO) {
        auto quote = event->getQuote();
        onBBO(event);
        LOGINFO("BBO Update Bid=" << quote->getBidSize() << '@' << quote->getBidPrice()
                    << " Ask=" << quote->getAskSize() << '@' << quote->getAskPrice()
                    << " " << LOG_NVP("Timestamp", quote->getTimestamp().to_string()));
    } else if (event->eventType() == EventType::TradeUpdate) {
        LOGDEBUG( "Trade: " << event->getTrade()->toJson().serialize());
        onTrade(event);
    } else if (event->eventType() == EventType::Exec) {
        LOGDEBUG("Execution: " << event->getExec()->toJson().serialize());
        _allocations->update(event->getExec());
        onExecution(event);
    }

    if (allocations()->modified()) {
        placeAllocations();
    }
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
            LOGINFO("Price level is occupied "<< LOG_VAR(order->getPrice())
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
                auto cancel = createOrder(allocation);
                // cancel will be for opposite side
                cancel->setSide("Buy" == allocation->getSide() ? "Sell" : "Buy");
                cancel->setOrderQty(0);
                cancel->setOrderID(currentOrder->getOrderID());
                _cancels.push_back(cancel);
                order->setOrderID("");
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
                order->setOrdStatus("PendingCancel");
                order->setOrderQty(0);
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
            _orderEngine->order_cancel(toSend->getOrderID(), toSend->getClOrdID(), std::string("Allocation removed")).then(
                [this, &toSend](const pplx::task<std::vector<std::shared_ptr<model::Order>>> &orders_) {
                    try {
                        this->updateFromTask(orders_);
                    } catch (api::ApiException &ex_) {
                        LOGERROR("Error cancelling order " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what())
                                    << LOG_NVP("order", toSend->toJson().serialize()));
                        _allocations->get(toSend->getPrice())->cancelDelta();
                    }
                });
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
        _orderEngine->order_amendBulk(jsList.serialize()).then(
            [this, &_amends](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                try {
                    this->updateFromTask(orders_);
                } catch (api::ApiException &ex_) {
                    LOGERROR("Error amending orders: " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what()));
                    _amends.clear();
                    _allocations->cancel([](const std::shared_ptr<Allocation> &alloc_) {
                        return alloc_->isAmendDown() || alloc_->isAmendUp();
                    });
                }
            });
    }

    // TODO: placeNewOrders method.
    jsList = web::json::value::array();
    count = 0;
    for (auto& toSend : _newOrders) {
        jsList.as_array()[count++] = toSend->toJson();
    }
    if (!_newOrders.empty()) {
        _orderEngine->order_newBulk(jsList.serialize()).then(
            [this, &_newOrders](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
                try {
                    this->updateFromTask(orders_);
                } catch (api::ApiException &ex_) {
                    LOGERROR("Error placing new orders " << ex_.getContent()->rdbuf() << LOG_VAR(ex_.what()));
                    _newOrders.clear();
                    _allocations->cancel([](const std::shared_ptr<Allocation>& alloc_) {return alloc_->isNew();});
                }
            });
    }

    LOGINFO("Allocations have been reflected. " << LOG_NVP("amend", _amends.size())
            << LOG_NVP("new", _newOrders.size()) << LOG_NVP("cancel", _cancels.size()));
    _allocations->restAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
    try {
        for (auto& order : task_.get()) {
            auto index = _allocations->allocIndex(order->getPrice());
            LOGINFO(order->toJson().serialize());
            _orders[index] = order;
        }
    } catch (api::ApiException apiException) {
        auto reason = apiException.getContent();
        LOGINFO("ApiException: " << apiException.getContent()->rdbuf() << " "
                << LOG_VAR(apiException.error_code())
                << LOG_VAR(apiException.what()));
        LOGINFO("APIException caught failed to action on order: " << LOG_VAR(apiException.what())
                                                             << LOG_NVP("Reason", reason->rdbuf()));
        throw apiException;
    } catch (web::http::http_exception& httpException) {
        LOGINFO("Failed to send order! " << LOG_VAR(httpException.what())
                                         << LOG_VAR(httpException.error_code()));
        throw httpException;
    }
}


#endif //TRADINGO_STRATEGY_H
