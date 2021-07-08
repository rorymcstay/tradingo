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


template<typename TOrdApi>
class Strategy {

    using OrderPtr = std::shared_ptr<model::Order>;

    std::shared_ptr<MarketDataInterface> _marketData;
    std::shared_ptr<TOrdApi>  _orderEngine;
    std::shared_ptr<model::Instrument> _instrument;


    // TODO Remove
    size_t _allocatedAsk = 0;
    size_t _allocatedBid = 0;

    std::string _symbol;
    std::string _clOrdIdPrefix;
    int _oidSeed;
    std::shared_ptr<Config> _config;

    std::vector<std::shared_ptr<Allocation>> _allocations;
    std::vector<OrderPtr> _buyOrders;
    std::vector<OrderPtr> _sellOrders;

    virtual void onExecution(const std::shared_ptr<Event>& event_) = 0;
    virtual void onTrade(const std::shared_ptr<Event>& event_) = 0;
    virtual void onBBO(const std::shared_ptr<Event>& event_) = 0;
    std::shared_ptr<model::Order> createOrder(const std::shared_ptr<Allocation>& allocation_);

    /// update orders after call to api.
    void updateFromTask(const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_);

public:
    std::vector<std::shared_ptr<Allocation>> allocations() { return _allocations; }
    Strategy(std::shared_ptr<MarketDataInterface> md_,  std::shared_ptr<TOrdApi> od_);
    void evaluate();

    void init(const std::string& config_);
    virtual void init(const std::shared_ptr<Config>& config_);

    virtual bool shouldEval();
protected:
    // allocation api
    void addAllocation(price_t price_, size_t qty_, const std::string& side_);
    size_t getAllocationIndex(price_t price_);
    void placeAllocations();

public:
    const std::shared_ptr<model::Instrument>& instrument() const { return _instrument; }
    void setInstrument(const std::shared_ptr<model::Instrument>& instr_) { _instrument = instr_; }
    price_t getNearestTickPrice(price_t price_);
};

template<typename TOrdApi>
Strategy<TOrdApi>::Strategy(std::shared_ptr<MarketDataInterface> md_, std::shared_ptr<TOrdApi> od_)
:   _marketData(std::move(md_))
,   _orderEngine(std::move(od_)) {

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
        LOGDEBUG("BBO Update Bid=" << quote->getBidSize() << '@' << quote->getBidPrice()
                    << " Ask=" << quote->getAskSize() << '@' << quote->getAskPrice());
    } else if (event->eventType() == EventType::TradeUpdate) {
        LOGDEBUG( "Trade: " << event->getTrade()->toJson().serialize());
        onTrade(event);
    } else if (event->eventType() == EventType::Exec) {
        LOGDEBUG("Execution: " << event->getExec()->toJson().serialize());
        onExecution(event);
    }
    if (_allocatedAsk > 0 || _allocatedBid > 0) {
        //
    } else {
        //createOrders(_bid, _ask);

    }
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::string& config_) {
    auto _config = std::make_shared<Config>(config_);
    init(_config);
    LOGINFO("Initializing strategy");
}

template<typename TOrdApi>
void Strategy<TOrdApi>::init(const std::shared_ptr<Config>& config_) {
    _config = config_;
    _symbol = _config->get("symbol");
    _clOrdIdPrefix = _config->get("clOrdPrefix");
    LOGINFO("Initializing strategy");
    for (auto& order : _marketData->getOrders())
        LOGINFO("Open Order: " << LOG_NVP("OID", order.first) << LOG_NVP("Order",order.second->toJson().serialize()));
    _allocations = std::vector<std::shared_ptr<Allocation>>(instrument()->getBidPrice()*2/instrument()->getTickSize(), nullptr);
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
    for (auto& allocation : _allocations) {
        if (!allocation) {
            continue;
        }
        std::shared_ptr<model::Order> order = createOrder(allocation);
        size_t priceIndex = getAllocationIndex(order->getPrice());
        auto& orders = (order->getSide() == "Buy") ? _buyOrders : _sellOrders;
        auto& currentOrder = orders[getAllocationIndex(order->getPrice())];
        if (currentOrder) {
            // we have an order at this price level
            if (currentOrder->getLeavesQty() > allocation->getSize()) {
                // amend up
                order->setOrdStatus("PendingAmend");
                order->setOrderID(currentOrder->getOrderID());
                order->setClOrdID(currentOrder->getClOrdID());
                _amends.push_back(order);
            } else if (currentOrder->getLeavesQty() < allocation->getSize()) {
                // amend down
                order->setOrdStatus("PendingAmend");
                order->setOrderID(currentOrder->getOrderID());
                order->setClOrdID(currentOrder->getClOrdID());
                _amends.push_back(order);
            } else if (allocation->getSize() == 0 and currentOrder->getLeavesQty() > 0) {
                // cancel order
                order->setOrdStatus("PendingCancel");
                order->setOrderQty(0);
                order->setOrderID(currentOrder->getOrderID());
                _cancels.push_back(order);
            }
        } else {
            _newOrders.push_back(order);
        }
        web::json::value jsList;
        int count = 0;
        for (auto& toSend : _newOrders) {
            jsList.as_array()[count++] = toSend->toJson();
        }
        _orderEngine->order_newBulk(jsList.serialize()).then(&this->updateFromTask);
        jsList = web::json::value();
        for (auto& toSend : _cancels) {
            jsList.as_array()[count++] = toSend->toJson();
        }
        _orderEngine->order_amendBulk(jsList.serialize()).then(&this->updateFromTask);
        jsList = web::json::value();
        for (auto& toSend : _amends) {
            jsList.as_array()[count++] = toSend->toJson();
        }
        _orderEngine->order_cancelBulk(jsList.serialize()).then(&this->updateFromTask);

    }
}


template<typename TOrdApi>
void Strategy<TOrdApi>::addAllocation(price_t price_, size_t qty_, const std::string& side_) {
    price_ = getNearestTickPrice(price_);
    LOGINFO("Adding allocation: " << LOG_VAR(price_) << LOG_VAR(qty_) << LOG_VAR(side_));
    auto& allocation = _allocations[getAllocationIndex(price_)];
    if (!allocation) {
        allocation = std::make_shared<Allocation>(price_, qty_, side_);
    } else {
        allocation->setSize(allocation->getSize() + qty_);
    }
}

template<typename TOrdApi>
std::shared_ptr<model::Order> Strategy<TOrdApi>::createOrder(const std::shared_ptr<Allocation> &allocation_) {
    auto newOrder = std::make_shared<model::Order>();
    newOrder->setPrice(allocation_->getPrice());
    newOrder->setSide(allocation_->getSide());
    newOrder->setOrderQty(allocation_->getSize());
    newOrder->setClOrdID(_clOrdIdPrefix + std::to_string(_oidSeed++));
    newOrder->setSymbol(_symbol);
    return newOrder;
}

template<typename TOrdApi>
size_t Strategy<TOrdApi>::getAllocationIndex(price_t price_) {
    int index = price_/_instrument->getTickSize();
    return index;
}

template<typename TOrdApi>
price_t Strategy<TOrdApi>::getNearestTickPrice(price_t price_) {
    /// TODO implement this correctly
    auto tickSize = _instrument->getTickSize();
    int ticksPerUnit = 10/tickSize;
    price_ *= ticksPerUnit;
    tickSize*10;
    return price_;
}

template<typename TOrdApi>
void Strategy<TOrdApi>::updateFromTask(const pplx::task<std::vector<std::shared_ptr<model::Order>>>& task_) {
    try {
        for (auto& order : task_.get()) {
            auto index = getAllocationIndex(order->getPrice());
            LOGINFO(order->toJson().serialize());
            auto& orders = (order->getSide() == "Buy") ? _buyOrders : _sellOrders;
            orders[index] = order;
        }
    } catch (api::ApiException &apiException) {
        auto reason = apiException.getContent();
        LOGINFO("APIException caught failed to action on order: " << LOG_VAR(apiException.what())
                                                             << " ExceptionContent="
                                                             << reason->rdbuf());
        return;
    } catch (web::http::http_exception &httpException) {
        LOGINFO("Failed to send order! " << LOG_VAR(httpException.what())
                                         << LOG_VAR(httpException.error_code()));
        return;
    }
}

#endif //TRADINGO_STRATEGY_H
