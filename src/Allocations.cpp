//
// Created by rory on 08/07/2021.
//
#include "Allocations.h"
#include "Utils.h"


Allocations::Allocations(price_t midPoint_, price_t tickSize_, qty_t lotSize_)
:   _data()
,   _tickSize(tickSize_)
,   _referencePrice(midPoint_)
,   _lowPrice(0.0)
,   _highPrice(0.0)
,   _lotSize(lotSize_)
,   _modified(false)
{
    for (int i=0; i < 2*(size_t)(midPoint_/tickSize_); i++) {
        _data.push_back(std::make_shared<Allocation>(i*tickSize_, 0.0));
    }

}

/// return the number of ticks in price
size_t Allocations::allocIndex(price_t price_) {
    int index = price_/_tickSize;
    return index;
}

/// allocate qty/price
void Allocations::addAllocation(price_t price_, qty_t qty_, const std::string& side_/*=""*/) {
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
    LOGINFO("Adding allocation: " << LOG_VAR(price_) << LOG_VAR(qty_) << LOG_VAR(side_));
    auto& allocation = _data[allocIndex(price_)];
    if (!allocation) {
        // shouldn't come in here.
        allocation = std::make_shared<Allocation>(price_, qty_);
    } else {
        allocation->setTargetDelta(qty_ + allocation->getTargetDelta());
    }
}

price_t Allocations::roundTickPassive(price_t price_) {
    return price_;
}

qty_t Allocations::allocatedAtLevel(price_t price_) {
    return _data[allocIndex(price_)]->getSize();
}

qty_t Allocations::totalAllocated() {
    qty_t allocated = 0;
    std::for_each(_data.begin(), _data.end(), [&allocated] (const decltype(_data)::value_type& alloc_) {
        allocated += alloc_->getSize();
    });
    return allocated;
}

const std::shared_ptr<Allocation>& Allocations::get(price_t price_) {
    return _data[allocIndex(price_)];
}

void Allocations::update(const std::shared_ptr<model::Execution> &exec_) {
    auto execType = exec_->getExecType();
    auto alloc = get(exec_->getPrice());
    if (execType == "New" || execType == "Replaced") {
        // do nothing - order placed
    } else if (execType == "Trade" || execType == "Canceled") {
        if (exec_->getSide() == "Buy") {
            // decrement
            alloc->reduce(exec_->getLastQty());
        } else {
            // increment
            alloc->reduce(-exec_->getLastQty());
        }
    } else {
        LOGWARN("Unhandled update " << LOG_VAR(execType));
        // unhandled update
    }
}

void Allocations::restAll() {
    std::for_each(_data.begin(), _data.end(), [](const std::shared_ptr<Allocation>& alloc_ ) {
        alloc_->rest();
    });
    setUnmodified();
}

void Allocations::rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(_data.begin(), _data.end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
        if (predicate_(alloc_))
            alloc_->rest();
    });
    setUnmodified();
}

void Allocations::cancel(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(_data.begin(), _data.end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
        if (predicate_(alloc_))
            LOGINFO("Allocations::cancel: Cancelling allocationDelta " << LOG_VAR(alloc_->getPrice()));
            alloc_->cancelDelta();
    });
    setUnmodified();
}

void Allocations::cancelOrders(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_) {

    std::for_each(_data.begin(), _data.end(), [predicate_](const std::shared_ptr<Allocation>& alloc_ ) {
        alloc_->setTargetDelta(-alloc_->getSize());
        if (predicate_(alloc_))
            LOGINFO("Allocations::cancelOrders: Cancelling allocation orders "
                            << LOG_NVP("Price",alloc_->getPrice())
                            << LOG_NVP("Side", alloc_->getSide())
                            << LOG_NVP("Size", alloc_->getSize()));
    });
    _modified = true;
}

qty_t Allocations::roundLotSize(qty_t size_) {
    if (size_ <= _lotSize)
        return 0.0;
    return size_ - ((int)size_ % (int)_lotSize);
}
