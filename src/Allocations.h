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



template<typename TOrdApi>
class Allocations {

private:
    std::vector<std::shared_ptr<Allocation>> _data;
    price_t _tickSize;
    price_t _referencePrice;
    price_t _lowPrice;
    price_t _highPrice;
    qty_t _lotSize;
    std::string _symbol;
    std::string _clOrdIdPrefix;

    std::vector<index_t> _occupiedLevels;

    bool _modified;

    std::shared_ptr<TOrdApi> _orderApi;
    void updateFromTask(const std::shared_ptr<Allocation>& allocation_,
    const std::shared_ptr<model::Order>&);
public:

    class iterator_type {

        std::vector<index_t>::const_iterator _index_iter;
        const Allocations<TOrdApi>* _series;

    public:

        iterator_type(
                std::vector<index_t>::const_iterator index_,
                const Allocations<TOrdApi>* series_
        )
        :   _index_iter(index_)
        ,   _series(series_) {}

        iterator_type(const iterator_type& iterator_type_)
        :   _index_iter(iterator_type_._index_iter)
        ,   _series(iterator_type_._series) {}

        iterator_type operator++() { _index_iter++; return *this; }
        bool operator!=(const iterator_type & other) const { 
            return other._index_iter != _index_iter;
        }
        const std::shared_ptr<Allocation>& operator*() const { return _series->_data[*_index_iter]; }

        Allocation* operator->() const { 
            auto& data = _series->_data;
            return data[*_index_iter].get();
        }

    };
    friend iterator_type;
    

    size_t allocIndex(price_t price_);
    Allocations(std::shared_ptr<TOrdApi> orderApi, std::string symbol_,
                std::string clOrdIdPrefix,
                int clOidSeed_,
                price_t midPoint_,
                price_t tickSize_,
                qty_t lotSize_);
    /// add to an allocation by price level.
    void addAllocation(price_t price_, qty_t qty_, const std::string& side_="");
    void addAllocation(const std::shared_ptr<Allocation>& allocation_);
    /// return the underlying data
    std::vector<std::shared_ptr<Allocation>> allocations() { return _data; }
    /// round price to tick.
    price_t roundTickPassive(price_t price_);
    /// round quantity to lot size.
    qty_t roundLotSize(qty_t size_);
    /// size at price level
    qty_t allocatedAtLevel(price_t price_);
    /// net total size exposure
    qty_t totalAllocated();
    /// return an allocation for price level
    const std::shared_ptr<Allocation>& get(price_t price_);
    void update(const std::shared_ptr<model::Execution>& exec_);
    /// if the set of allocations has changed
    bool modified() { return _modified; }
    void setUnmodified() { _modified = false; }
    const std::shared_ptr<Allocation>& operator[] (price_t price_) { return get(price_); }
    /// iterators
    iterator_type begin() const;
    iterator_type end() const;
    size_t size() const ;
    /// declare all allocations fulfilled.
    void restAll();
    /// declare allocations fulfilled
    void rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_);
    /// reset the desired allocation delta.
    void cancel(const std::function<bool(const std::shared_ptr<Allocation>& )>& predicate_);
    /// cancel all price levels
    void cancelOrders(const std::function<bool(const std::shared_ptr<Allocation> &)> &predicate_);
    void placeAllocations();

};


template<typename TOrdApi>
Allocations<TOrdApi>::Allocations(std::shared_ptr<TOrdApi> orderApi_,
                                  std::string symbol_,
                                  std::string clOrdIdPrefix_,
                                  int clOidSeed_,
                                  price_t midPoint_, price_t tickSize_,
                                  qty_t lotSize_)
    :   _data()
    ,   _tickSize(tickSize_)
    ,   _referencePrice(midPoint_)
    ,   _lowPrice(0.0)
    ,   _highPrice(0.0)
    ,   _lotSize(lotSize_)
    ,   _modified(false)
    ,   _orderApi(orderApi_)
    ,   _symbol(std::move(symbol_))
    ,   _clOrdIdPrefix(std::move(clOrdIdPrefix_))
    ,   _occupiedLevels()
{
    for (int i=0; i < 2*(size_t)(midPoint_/tickSize_); i++) {
        _data.push_back(std::make_shared<Allocation>(i*tickSize_, 0.0));
    }

}


/// return the number of ticks in price
template<typename TOrdApi>
size_t Allocations<TOrdApi>::allocIndex(price_t price_) {
    int index = price_/_tickSize;
    return index;
}

/// allocate qty/price
template<typename TOrdApi>
void Allocations<TOrdApi>::addAllocation(const std::shared_ptr<Allocation>& allocation_) {
    addAllocation(allocation_->getPrice(), allocation_->getTargetDelta());
}

/// allocate qty/price
template<typename TOrdApi>
void Allocations<TOrdApi>::addAllocation(price_t price_, qty_t qty_, const std::string& side_/*=""*/) {

    if (almost_equal(qty_,0.0)) {
        return;
    }
    if (price_ < _lowPrice) {
        _lowPrice = price_;
    } else if (price_ > _highPrice) {
        _highPrice = price_;
    }
    _modified = true;
    price_ = roundTickPassive(price_);
    if (qty_ > 0 && side_ == "Sell") {
        qty_ = - qty_;
    }
    auto price_index = allocIndex(price_);
    auto& allocation = _data[price_index];
    // mark occupied if weve not done so
    if (std::find(_occupiedLevels.begin(), _occupiedLevels.end(), price_index) == _occupiedLevels.end()) {
        _occupiedLevels.push_back(price_index);
        std::sort(_occupiedLevels.begin(), _occupiedLevels.end());
    }
    if (!allocation) {
        // shouldn't come in here.
        allocation = std::make_shared<Allocation>(price_, qty_);
    } else {
        allocation->setTargetDelta(qty_ + allocation->getTargetDelta());
        allocation->getOrder()->setSymbol(_symbol);
    }
    LOGINFO("Adding allocation: "
            << LOG_VAR(price_)
            << LOG_VAR(qty_)
            << LOG_VAR(side_)
            << LOG_VAR(allocation->getTargetDelta())
            << LOG_VAR(allocation->getSize()));
}


template<typename TOrdApi>
price_t Allocations<TOrdApi>::roundTickPassive(price_t price_) {
    return price_;
}


template<typename TOrdApi>
qty_t Allocations<TOrdApi>::allocatedAtLevel(price_t price_) {
    return _data[allocIndex(price_)]->getSize();
}


template<typename TOrdApi>
qty_t Allocations<TOrdApi>::totalAllocated() {
    qty_t allocated = 0;
    std::for_each(begin(), end(), [&allocated] (const typename decltype(_data)::value_type& alloc_) {
      allocated += alloc_->getSize();
    });
    return allocated;
}


template<typename TOrdApi>
const std::shared_ptr<Allocation>& Allocations<TOrdApi>::get(price_t price_) {
    return _data[allocIndex(price_)];
}


template<typename TOrdApi>
void Allocations<TOrdApi>::update(const std::shared_ptr<model::Execution> &exec_) {
    // at this point the underlying order of the allocation is out of date.
    // it should not be read from and instead the order in the market data should
    // be used for status of orders.

    auto alloc = get(exec_->getPrice());
    alloc->update(exec_);
    if (almost_equal(exec_->getLeavesQty(), 0.0)) {

        auto ref = std::find(_occupiedLevels.begin(), _occupiedLevels.end(), allocIndex(exec_->getPrice()));
        if (ref != _occupiedLevels.end())
            _occupiedLevels.erase(ref);
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
void Allocations<TOrdApi>::rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (predicate_(alloc_))
          alloc_->rest();
    });
    setUnmodified();
}


template<typename TOrdApi>
void Allocations<TOrdApi>::cancel(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (predicate_(alloc_))
          LOGINFO("cancel: Cancelling allocationDelta " << LOG_VAR(alloc_->getPrice()));
      alloc_->cancelDelta();
    });
    setUnmodified();
}


template<typename TOrdApi>
void Allocations<TOrdApi>::cancelOrders(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(begin(), end(), [this, predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
      if (predicate_(alloc_)) {
          alloc_->setTargetDelta(-alloc_->getSize());
          LOGINFO("cancelOrders: Cancelling allocation orders "
                  << LOG_NVP("Price", alloc_->getPrice())
                  << LOG_NVP("Side", alloc_->getSide())
                  << LOG_NVP("Size", alloc_->getSize()));
          _modified = true;
      }
    });

}


template<typename TOrdApi>
qty_t Allocations<TOrdApi>::roundLotSize(qty_t size_) {
    if (size_ <= _lotSize)
        return 0.0;
    return size_ - ((int)size_ % (int)_lotSize);
}


template<typename TOrdApi>
void Allocations<TOrdApi>::placeAllocations() {
    std::vector<std::shared_ptr<model::Order>> _amends;
    std::vector<std::shared_ptr<model::Order>> _newOrders;
    std::vector<std::shared_ptr<model::Order>> _cancels;
    _amends.clear(); _newOrders.clear(); _cancels.clear();
    setUnmodified();
    // TODO use ranges ontop of *_allocations iterator. - occupied price level view
    for (auto occupiedPrice : _occupiedLevels) {
        auto& allocation = _data[occupiedPrice];
        if (!allocation) {
            continue;
        }
        if (almost_equal(allocation->getTargetDelta(), 0.0)) {
            continue;
        }
        LOGINFO("Processing allocation "
                     << LOG_NVP("targetDelta",allocation->getTargetDelta())
                     << LOG_NVP("currentSize", allocation->getSize())
                     << LOG_NVP("price",allocation->getPrice()));

        auto& order = allocation->getOrder();

        std::stringstream actionMessage;

        if (allocation->isNew()) {
            actionMessage << " PLACING_NEW ";
            order->setSymbol(_symbol);
            order->setClOrdID(_clOrdIdPrefix +
                              std::to_string(allocation->getPrice()) + "_v" +
                              std::to_string(allocation->getVersion()));
            try {
                order->setSymbol(_symbol);
                _orderApi->order_new(order->getSymbol(),
                                order->getSide(),
                                boost::none, // simpleOrderQty
                                order->getOrderQty(),
                                order->getPrice(),
                                boost::none, // displayQty
                                boost::none, // stopPx
                                order->getClOrdID(),
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
            } catch (api::ApiException& ex_) {
                LOGERROR("Error placing new order "
                         << LOG_NVP("response", ex_.getContent()->rdbuf())
                         << LOG_VAR(ex_.what())
                         << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
                allocation->getOrder()->setOrderQty(0.0);
            }
        } else if (allocation->isChangingSide()) {
            actionMessage << " CHANGING_SIDES ";
            auto newQty = order->getOrderQty();
            order->setClOrdID(_clOrdIdPrefix
                              + std::to_string(allocation->getPrice())
                              + "_v"
                              + std::to_string(allocation->getVersion())
            );
            try {
                auto task = _orderApi->order_cancel(
                    boost::none, // orderId
                    order->getOrigClOrdID(),
                    boost::none  // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::vector<std::shared_ptr<model::Order>>> &order_) {
                      this->updateFromTask(allocation, order_.get()[0]);
                    });
                task.wait();
            } catch (api::ApiException &ex_) {
                LOGERROR("Error cancelling order "
                             << LOG_NVP("response", ex_.getContent()->rdbuf())
                             << LOG_VAR(ex_.what())
                             << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
            }
            try {
                order->setSymbol(_symbol);
                auto task = _orderApi->order_new(
                    order->getSymbol(),
                    allocation->targetSide(),
                    boost::none, // simpleOrderQty
                    newQty,
                    order->getPrice(),
                    boost::none, // displayQty
                    boost::none, // stopPx
                    order->getClOrdID(),
                    boost::none, // clOrdLinkId
                    boost::none, // pegOffsetValue
                    boost::none, // pegPriceType
                    order->getOrdType(),
                    order->getTimeInForce(),
                    boost::none, // execInst
                    boost::none, // contingencyType
                    boost::none  // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::shared_ptr<model::Order>> &order_) {
                      this->updateFromTask(allocation, order_.get());
                    });
                task.wait();
            } catch (api::ApiException &ex_) {
                LOGERROR("Error placing new order "
                             << LOG_NVP("response", ex_.getContent()->rdbuf())
                             << LOG_VAR(ex_.what())
                             << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
            }
        } else if (allocation->isAmendDown() || allocation->isAmendUp()) {
            actionMessage << " AMENDING ";
            auto old_orig_clid = order->getOrigClOrdID();
            auto orig_was_set = order->origClOrdIDIsSet();
            auto origClOrdId = order->getClOrdID();
            order->setClOrdID(_clOrdIdPrefix
                              + std::to_string(allocation->getPrice())
                              + "_v"
                              + std::to_string(allocation->getVersion())
            );
            order->setOrigClOrdID(origClOrdId);
            try {
                auto task = _orderApi->order_amend(
                    boost::none, // orderId
                    order->getOrigClOrdID(),
                    order->getClOrdID(),
                    boost::none, // simpleOrderQty,
                    boost::none, // orderQty,
                    boost::none, // simpleLeavesQty,
                    order->getLeavesQty(),
                    boost::none, // price
                    boost::none, // stopPx
                    boost::none, // pegOffsetValue
                    boost::none // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::shared_ptr<model::Order>> &order_) {
                      this->updateFromTask(allocation, order_.get());
                    });
                task.wait();
            } catch (api::ApiException &ex_) {
                order->setClOrdID(order->getOrigClOrdID());
                order->setOrigClOrdID(old_orig_clid);
                if (not orig_was_set) {
                    order->unsetOrigClOrdID();
                }
                LOGERROR("Error amending order "
                             << LOG_NVP("response", ex_.getContent()->rdbuf())
                             << LOG_VAR(ex_.what())
                             << LOG_NVP("action", actionMessage.str()));
                allocation->cancelDelta();
            }
        } else if (allocation->isCancel()) {
            actionMessage << " CANCELLING ";
            order->setClOrdID(_clOrdIdPrefix
                              + std::to_string(allocation->getPrice())
                              + "_v"
                              + std::to_string(allocation->getVersion())
            );
            try {
                auto task = _orderApi->order_cancel(
                    boost::none, // orderId
                    order->getClOrdID(),
                    boost::none  // text
                ).then(
                    [this, allocation, &order](const pplx::task<std::vector<std::shared_ptr<model::Order>>> &order_) {

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
            }
        } else {
            LOGERROR("Couldn't determine action for " << actionMessage.str());
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
    LOGINFO("Success: " << LOG_NVP("price", order_->getPrice())
                        << LOG_NVP("ordStatus", order_->getOrdStatus())
                        << LOG_NVP("orderQty", order_->getOrderQty())
                        << LOG_NVP("leavesQty",order_->getLeavesQty())
                        << LOG_NVP("side", order_->getSide())
                        << LOG_NVP("cumQty", order_->getCumQty()));
    if (allocation_->isChangingSide() and order_->getOrdStatus() == "Cancelled") {
        allocation_->setTargetDelta(allocation_->getSize() + allocation_->getTargetDelta());
    } if (allocation_->isCancel()) {
        allocation_->setSize(0.0);
        _occupiedLevels.erase(
                std::find(_occupiedLevels.begin(), _occupiedLevels.end(), allocIndex(allocation_->getPrice()))
        );

    }

}


template<typename TOrdApi>
size_t Allocations<TOrdApi>::size() const {
    return _occupiedLevels.size();
}


template<typename TOrdApi>
typename Allocations<TOrdApi>::iterator_type
Allocations<TOrdApi>::begin() const {
    return Allocations<TOrdApi>::iterator_type(_occupiedLevels.begin(), this);
}


template<typename TOrdApi>
typename Allocations<TOrdApi>::iterator_type
Allocations<TOrdApi>::end() const {
    return Allocations<TOrdApi>::iterator_type(_occupiedLevels.end(), this);
}


#endif //MY_PROJECT_ALLOCATIONS_H
