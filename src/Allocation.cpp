//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"
#include "Utils.h"
#include <stdexcept>


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
    _size = _order->getLeavesQty();
    _targetDelta = 0;
}


void Allocation::cancelDelta() {
    if (isAmendUp() || isAmendDown()) {
        _version--;
    }
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
        _order->setSide(targetSide());
        _order->setOrdStatus("PendingNew");
        _order->setLeavesQty(std::abs(_targetDelta));
    } else if (isAmendUp() || isAmendDown()) {
        if(_order->getOrdStatus() != "PendingAmend")
            _version++;
        _order->setOrdStatus("PendingAmend");
        _order->setLeavesQty(std::abs(_targetDelta + _size));
        _order->setOrderQty(_order->getCumQty() + std::abs(_size + _targetDelta));
    } else if (isCancel()) {
        _order->setOrdStatus("PendingCancel");
        _order->setLeavesQty(0.0);
    } else if (isChangingSide()) {
        double tgtQty = getTargetDelta();
        _order->setSide(targetSide());
        _order->setOrderQty(std::abs(_size + _targetDelta));
        _order->setOrigClOrdID(_order->getClOrdID());
        _order->setOrdStatus("ChangingSides");
    } else {
        throw std::runtime_error("No Action!");
    }
}


void Allocation::update(const std::shared_ptr<model::Execution>& exec_) {
    _order->setLeavesQty(exec_->getLeavesQty());
    _order->setCumQty(exec_->getCumQty());
    _order->setAvgPx(exec_->getAvgPx());
    _order->setOrderQty(exec_->getOrderQty());
    _order->setOrdStatus(exec_->getOrdStatus());
    _size = (exec_->getSide() == "Buy" ? 1 : -1 ) * exec_->getLeavesQty();
    LOGINFO("Updated allocation " << LOG_VAR(_price) << LOG_VAR(_size) << LOG_VAR(_order->getOrdStatus()));
    if (tradingo_utils::almost_equal(_size, 0.0)) {
        LOGINFO("Order is complete " << LOG_NVP("ClOrdID", _order->getClOrdID()));
        _version++;
    }
}
