//
// Created by rory on 08/07/2021.
//

#ifndef MY_PROJECT_ALLOCATIONS_H
#define MY_PROJECT_ALLOCATIONS_H

#include <algorithm>
#include <memory>
#include <boost/optional.hpp>
#include <utility>


#include "Utils.h"
#include "ApiException.h"

#include "Allocation.h"
#include "PriceLevelContainer.h"

using namespace tradingo_utils;


template<typename TOrdApi>
class Allocations : public PriceLevelContainer<Allocation> {

private:
    std::shared_ptr<TOrdApi> _orderApi;
    bool _modified;
    std::string _clOrdIdPrefix;
    std::string _symbol;
public:
    Allocations(std::shared_ptr<TOrdApi> orderApi, std::string symbol_,
                std::string clOrdIdPrefix,
                int clOidSeed_,
                price_t midPoint_,
                price_t tickSize_,
                qty_t lotSize_);

    /// update from call to order api
    void updateFromTask(const std::shared_ptr<Allocation>& allocation_,
                        const std::shared_ptr<model::Order>&);
    /// declare allocations fulfilled
    void rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_);
    /// declare all allocations fulfilled.
    void restAll();
    /// add to an allocation by price level.
    void addAllocation(price_t price_, qty_t qty_, const std::string& side_="");
    /// add and merge allocation
    void addAllocation(const std::shared_ptr<Allocation>& allocation_);
    /// size at price level
    qty_t allocatedAtLevel(price_t price_);
    /// net total size exposure
    qty_t totalAllocated();
    /// update an allocation from execution
    void update(const std::shared_ptr<model::Execution>& exec_);
    /// if the set of allocations has changed
    bool modified() { return _modified; }
    /// declare allocations unmodified
    void setUnmodified() { _modified = false; }

    /// cancel all price levels
    void cancelOrders(const std::function<bool(const std::shared_ptr<Allocation> &)> &predicate_);
    void cancel(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_);
    void placeAllocations();

};


template<typename TOrdApi>
Allocations<TOrdApi>::Allocations(std::shared_ptr<TOrdApi> orderApi_,
                std::string symbol_,
                std::string clOrdIdPrefix_,
                int clOidSeed_,
                price_t midPoint_,
                price_t tickSize_,
                qty_t lotSize_)
:   PriceLevelContainer<Allocation>(midPoint_, tickSize_, lotSize_)
,   _orderApi(orderApi_)
,   _clOrdIdPrefix(clOrdIdPrefix_)
,   _modified(false)
,   _symbol(symbol_)
{}


/// allocate qty/price
template<typename TOrdApi>
void Allocations<TOrdApi>::addAllocation(
        const std::shared_ptr<Allocation>& allocation_
    ) {
    addAllocation(allocation_->getPrice(), allocation_->getTargetDelta());
}

/// allocate qty/price
template<typename TOrdApi>
void Allocations<TOrdApi>::addAllocation(
        price_t price_,
        qty_t qty_,
        const std::string& side_/*=""*/
    ) {

    if (almost_equal(qty_,0.0)) {
        return;
    }
    if (price_ < _lowPrice) {
        _lowPrice = price_;
    } else if (price_ > _highPrice) {
        _highPrice = price_;
    }
    _modified = true;
    price_ = PriceLevelContainer<Allocation>::roundTickPassive(price_);
    if (qty_ > 0 && side_ == "Sell") {
        qty_ = - qty_;
    }
    auto price_index = allocIndex(price_);
    auto& allocation = get(price_);
    PriceLevelContainer::occupy(price_);
    allocation->setTargetDelta(qty_ + allocation->getTargetDelta());
    allocation->getOrder()->setSymbol(_symbol);
    LOGINFO("Adding allocation: "
            << LOG_VAR(price_)
            << LOG_VAR(qty_)
            << LOG_VAR(side_)
            << LOG_VAR(allocation->getTargetDelta())
            << LOG_VAR(allocation->getSize()));
}


template<typename TOrdApi>
qty_t Allocations<TOrdApi>::allocatedAtLevel(price_t price_) {
    return get(price_)->getSize();
}


template<typename TOrdApi>
qty_t Allocations<TOrdApi>::totalAllocated() {
    qty_t allocated = 0;
    std::for_each(begin(), end(), [&allocated] (const std::shared_ptr<Allocation>& alloc_) {
      allocated += alloc_->getSize();
    });
    return allocated;
}



template<typename TOrdApi>
void Allocations<TOrdApi>::update(const std::shared_ptr<model::Execution> &exec_) {
    // at this point the underlying order of the allocation is out of date.
    // it should not be read from and instead the order in the market data should
    // be used for status of orders.

    auto alloc = get(exec_->getPrice());
    alloc->update(exec_);
    if (almost_equal(exec_->getLeavesQty(), 0.0)) {
        PriceLevelContainer::unoccupy(exec_->getPrice());
    }
}


template<typename TOrdApi>
void Allocations<TOrdApi>::restAll() {
    std::for_each(begin(), end(), [](const std::shared_ptr<Allocation>& alloc_ ) {
      alloc_->rest();
    });
    setUnmodified();
}


template<typename TOrdApi>
void Allocations<TOrdApi>::rest(
        const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (predicate_(alloc_))
          alloc_->rest();
    });
    setUnmodified();
}


template<typename TOrdApi>
void Allocations<TOrdApi>::cancel(
        const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (predicate_(alloc_))
          LOGINFO("cancel: Cancelling allocationDelta " << LOG_VAR(alloc_->getPrice()));
      alloc_->cancelDelta();
    });
    setUnmodified();
}


template<typename TOrdApi>
void Allocations<TOrdApi>::cancelOrders(
        const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [this, predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (alloc_->getSize() != 0.0 and predicate_(alloc_)) {
          alloc_->setTargetDelta(-alloc_->getSize());
          LOGINFO("cancelOrders: Cancelling allocation orders "
                  << LOG_NVP("OrderID", alloc_->getOrder()->getOrderID())
                  << LOG_NVP("Price", alloc_->getPrice())
                  << LOG_NVP("Side", alloc_->getSide())
                  << LOG_NVP("Size", alloc_->getSize()));
          _modified = true;
      }
    });

}



template<typename TOrdApi>
void Allocations<TOrdApi>::placeAllocations() {

    setUnmodified();

    for (auto price_level : _occupiedLevels) {
        auto & allocation = get(price_level);
        if (almost_equal(allocation->getTargetDelta(), 0.0)) {
            continue;
        }
        auto order = allocation->getOrder();
        LOGINFO("Processing allocation "
                     << LOG_NVP("targetDelta", allocation->getTargetDelta())
                     << LOG_NVP("currentSize", allocation->getSize())
                     << LOG_NVP("price", allocation->getPrice())
                     << LOG_NVP("clOrdID", order->getClOrdID())
                     << LOG_NVP("ordStatus", order->getOrdStatus())
                     << LOG_NVP("leavesQty", order->getLeavesQty())
                     << LOG_NVP("orderQty", order->getCumQty())
                );

        std::stringstream actionMessage;
        if (allocation->isChangingSide() or allocation->isCancel()) {
            actionMessage << ((  allocation->isChangingSide()) ? "CHANGING SIDE " : "") << "CANCEL";
            try {
                auto task = _orderApi->order_cancel(
                    boost::none, // orderId
                    order->getClOrdID(),
                    boost::none  // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::vector<std::shared_ptr<model::Order>>>
                                                            &order_) {
                            this->updateFromTask(allocation, order_.get()[0]);
                    });
                task.wait();
            } catch (api::ApiException &ex_) {
                LOGERROR("Error cancelling order "
                         << LOG_NVP("response", ex_.getContent()->rdbuf())
                         << LOG_VAR(ex_.what())
                         << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
                allocation->getOrder()->setOrderQty(0.0);
                allocation->getOrder()->setOrdStatus("Rejected");
                continue;
            }
        }

        if (allocation->isChangingSide() or allocation->isNew()) {
            actionMessage << ((  allocation->isChangingSide()) ? "CHANGING SIDE " : "") << "PLACING NEW";
            try {
                auto task = _orderApi->order_new(order->getSymbol(),
                                allocation->targetSide(),
                                boost::none, // simpleOrderQty
                                std::abs(allocation->getTargetDelta()),
                                allocation->getPrice(),
                                boost::none, // displayQty
                                boost::none, // stopPx
                                allocation->makeClOrdID(_clOrdIdPrefix),
                                boost::none, // clOrdLinkId
                                boost::none, // pegOffsetValue
                                boost::none, // pegPriceType
                                order->getOrdType(),
                                order->getTimeInForce(),
                                boost::none, // execInst
                                boost::none, // contingencyType
                                boost::none  // text
                            )
                    .then([this, allocation, &order](
                              const pplx::task<std::shared_ptr<model::Order>>&
                                  order_) {
                        this->updateFromTask(allocation, order_.get());
                    });
                task.wait();
            } catch (api::ApiException& ex_) {
                LOGERROR("Error placing new order "
                         << LOG_NVP("response", ex_.getContent()->rdbuf())
                         << LOG_VAR(ex_.what())
                         << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
            }
        } else if (allocation->isAmendDown() || allocation->isAmendUp()) {
            actionMessage << "AMENDING";
            try {
                auto task = _orderApi->order_amend(
                    boost::none, // orderId
                    order->getClOrdID(),
                    allocation->makeClOrdID(_clOrdIdPrefix),
                    boost::none, // simpleOrderQty,
                    boost::none, // orderQty,
                    boost::none, // simpleLeavesQty,
                    std::abs(allocation->getTargetDelta()+allocation->getSize()),
                    boost::none, // price
                    boost::none, // stopPx
                    boost::none, // pegOffsetValue
                    boost::none // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::shared_ptr<model::Order>>
                                                        &order_) {
                      this->updateFromTask(allocation, order_.get());
                    });
                task.wait(); // this can be done async for all allocs
            } catch (api::ApiException &ex_) {
                LOGERROR("Error amending order "
                             << LOG_NVP("response", ex_.getContent()->rdbuf())
                             << LOG_VAR(ex_.what())
                             << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
            }
        }

        LOGINFO(LOG_NVP("ClOrdID", order->getClOrdID()) 
             << LOG_NVP("OrigClOrdID",order->getOrigClOrdID())
             << LOG_NVP("Symbol", order->getSymbol())
             << LOG_NVP("OrdStatus", order->getOrdStatus())
             << LOG_NVP("OrderQty", order->getOrderQty())
             << LOG_NVP("LeavesQty", order->getLeavesQty())
             << LOG_NVP("Price", order->getPrice())
             << LOG_NVP("Action", actionMessage.str()));
    }

    restAll();

}


template<typename TOrdApi>
void Allocations<TOrdApi>::updateFromTask(const std::shared_ptr<Allocation>& allocation_,
                                          const std::shared_ptr<model::Order>& order_) {
    allocation_->setOrder(order_);
    LOGINFO("Success: " 
                        << LOG_NVP("clOrdID", order_->getClOrdID())
                        << LOG_NVP("origClOrdID", order_->getOrigClOrdID())
                        << LOG_NVP("price", order_->getPrice())
                        << LOG_NVP("ordStatus", order_->getOrdStatus())
                        << LOG_NVP("orderQty", order_->getOrderQty())
                        << LOG_NVP("leavesQty",order_->getLeavesQty())
                        << LOG_NVP("side", order_->getSide())
                        << LOG_NVP("cumQty", order_->getCumQty()));
    if (allocation_->isChangingSide() and order_->getOrdStatus() == "Canceled") {
        allocation_->setTargetDelta(allocation_->getSize() + allocation_->getTargetDelta());
        allocation_->setSize(0.0);
    }
    if (allocation_->isCancel()) {
        allocation_->rest();
        allocation_->setSize(0.0);
        PriceLevelContainer::unoccupy(allocation_->getPrice());
    }

}




#endif //MY_PROJECT_ALLOCATIONS_H
