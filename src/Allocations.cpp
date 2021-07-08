//
// Created by rory on 08/07/2021.
//

#include "Allocations.h"
#include "Utils.h"

Allocations::Allocations(price_t midPoint_, price_t tickSize_)
:   _data(2*(size_t)(midPoint_/tickSize_), std::make_shared<Allocation>())
,   _tickSize(tickSize_) {

}

/// return the number of ticks in price
size_t Allocations::allocIndex(price_t price_) {
    int index = price_/_tickSize;
    return index;
}

/// allocate qty/price
void Allocations::addAllocation(price_t price_, size_t qty_, const std::string& side_="") {
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
        allocation->setSize(allocation->getSize() + qty_);
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
    std::for_each(_data.begin(), _data.end(), [&allocated] (decltype(_data)::value_type alloc_) {
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
    if (execType == "New") {
        // do nothing - order placed
    } else if (execType == "Trade" || execType == "Cancelled") {
        if (exec_->getSide() == "Buy") {
            // decrement
            alloc->setSize(alloc->getSize() + exec_->getLastQty());
        } else {
            // increment
            alloc->setSize(alloc->getSize() - exec_->getLastQty());
        }
    } else {
        // unhandled update
    }
}
