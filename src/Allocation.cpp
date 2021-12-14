//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"
#include "Utils.h"


Allocation::Allocation()
:   _price(0.0)
,   _size(0.0)
,   _version(0)
,   _order(std::make_shared<model::Order>()) {

    _order->setOrdType("Limit");
    setTargetDelta(0.0);
}


Allocation::Allocation(price_t price_, size_t qty_)
:   _price(price_)
,   _size(0)
,   _version(0)
,   _order(std::make_shared<model::Order>()) {
    _order->setOrdType("Limit");
    setTargetDelta(qty_);
}


void Allocation::rest() {
    _size += _targetDelta;
    _targetDelta = 0;
}


void Allocation::cancelDelta() {
    _targetDelta = 0;
}


std::string Allocation::getSide() const {
    if (almost_equal(_size, 0.0))
        return "";
    else
        return (less_equal(_size, 0.0)) ? "Sell" : "Buy";
}


void Allocation::setTargetDelta(qty_t delta_) {
    _targetDelta = delta_;
    if (almost_equal(delta_,0.0)) {
        return;
    }
    if (almost_equal(_size,  0.0)) { // isNew()
        // order is pending new
        _order->setOrderQty(std::abs(_targetDelta));
        _order->setPrice(_price);
        _order->setSide(_targetDelta >= 0 ? "Buy" : "Sell");
        _order->setOrdStatus("PendingNew");
        _order->setLeavesQty(std::abs(_targetDelta));
    } else if (isAmendUp() || isAmendDown()) {
        _version++;
        _order->setOrdStatus("PendingAmend");
        double newLeavesQty = _order->getLeavesQty();
        newLeavesQty += ((isAmendUp()) ? 1 : -1)* std::abs(_targetDelta);
        _order->setLeavesQty(newLeavesQty);
        _order->setOrigClOrdID(_order->getClOrdID());
    } else if (isCancel()) {
        _order->setOrdStatus("PendingCancel");
        _order->setLeavesQty(0.0);
    } else if (isChangingSide()) {
        double tgtQty = getTargetDelta();
        _order->setSide(targetSide());
        _order->setOrderQty(std::abs(_size + _targetDelta));
        _order->setOrigClOrdID(_order->getClOrdID());
        _order->setOrdStatus("ChangingSides");
    }
}


void Allocation::update(const std::shared_ptr<model::Execution>& exec_) {
    _order->setLeavesQty(exec_->getLeavesQty());
    _order->setCumQty(exec_->getCumQty());
    _order->setAvgPx(exec_->getAvgPx());
    _order->setOrdStatus(exec_->getOrdStatus());

    auto execType = exec_->getExecType();
    if (execType == "New" || execType == "Replaced") {
        // do nothing - order placed
    } else if (execType == "Trade" || execType == "Canceled") {
        if (exec_->getSide() == "Buy") {
            // decrement
            reduce(exec_->getLastQty());
        } else {
            // increment
            reduce(-exec_->getLastQty());
        }
    }
}