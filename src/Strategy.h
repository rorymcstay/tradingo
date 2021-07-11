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
    std::unordered_map<price_t, OrderPtr> _buyOrders;
    std::unordered_map<price_t, OrderPtr> _sellOrders;
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
    const std::shared_ptr<MarketDataInterface> getMD() const { return _marketData; }

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
,   _buyOrders()
,   _sellOrders() {

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
    double tickSize;
    double referencePrice;
    tickSize = instrument()->getTickSize();
    referencePrice = instrument()->getPrevPrice24h();

    LOGINFO("Initialising allocations with " << LOG_VAR(referencePrice) << LOG_VAR(tickSize));
    _allocations = std::make_shared<Allocations>(referencePrice, tickSize);

}

template<typename TOrdApi>
bool Strategy<TOrdApi>::shouldEval() {
    return true;
}


template<typename TOrdApi>
void Strategy<TOrdApi>::placeAllocations() {
    thread_local std::vector<std::shared_ptr<model::Order>> _amends;
    thread_local std::vector<std::shared_ptr<model::Order>> _newOrders;
    thread_local std::vector<std::shared_ptr<model::Order>> _cancels;
    _amends.clear(); _newOrders.clear(); _cancels.clear();
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
            LOGINFO("Price level is occupied "<< LOG_VAR(order->getPrice()) << LOG_VAR(order->getOrderQty())
                    << LOG_VAR(currentOrder->getOrderID()));
            order->setOrderID(currentOrder->getOrderID());
            order->setClOrdID(currentOrder->getClOrdID());
            // order->setOrigClOrdID(currentOrder->getOrigClOrdID());
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
                LOGINFO("Amending down");
                order->setOrdStatus("PendingAmend");
                _amends.push_back(order);
            } else if (allocation->isAmendUp()) {
                LOGINFO("Amending up");
                order->setOrdStatus("PendingAmend");
                _amends.push_back(order);
            } else if (allocation->isCancel()) {
                // cancel order
                LOGINFO("Cancelling");
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
        }
    }


    auto taskUpdateFunc = [this](const pplx::task<std::vector<std::shared_ptr<model::Order>>>& orders_) {
        this->updateFromTask(orders_);
    };
    // make cancels first
    for (auto& toSend : _cancels) {
        _orderEngine->order_cancel(toSend->getOrderID(), toSend->getClOrdID(), std::string("Allocation removed")).then(taskUpdateFunc);
    }

    auto jsList = web::json::value::array();
    int count = 0;
    for (auto& toSend : _amends) {
        jsList.as_array()[count++] = toSend->toJson();
    }
    // and then amends
    if (!_amends.empty())
        _orderEngine->order_amendBulk(jsList.serialize()).then(taskUpdateFunc);

    jsList = web::json::value::array();
    count = 0;
    for (auto& toSend : _newOrders) {
        jsList.as_array()[count++] = toSend->toJson();
    }
    if (!_newOrders.empty())
        _orderEngine->order_newBulk(jsList.serialize()).then(taskUpdateFunc);

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
    } catch (api::ApiException &apiException) {
        auto reason = apiException.getContent();
        LOGINFO("APIException caught failed to action on order: " << LOG_VAR(apiException.what())
                                                             << LOG_NVP("Reason", reason->rdbuf()));
        return;
    } catch (web::http::http_exception &httpException) {
        LOGINFO("Failed to send order! " << LOG_VAR(httpException.what())
                                         << LOG_VAR(httpException.error_code()));
        return;
    }
}


#endif //TRADINGO_STRATEGY_H
