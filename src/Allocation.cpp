//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"
#include "Utils.h"
#include <stdexcept>


Allocation::Allocation()
:   _price(0.0)
,   _size(0.0)
,   _order(std::make_shared<model::Order>()) {

    _order->setOrdType("Limit");
    setTargetDelta(0.0);
}


Allocation::Allocation(price_t price_, size_t qty_)
:   _price(price_)
,   _size(0)
,   _order(std::make_shared<model::Order>()) {
    _order->setOrdType("Limit");
    setTargetDelta(qty_);
}


void Allocation::rest() {
    if (_order->getOrdStatus() == "Canceled") {
        _size = 0;
    } else {
        _size = (_order->getSide() == "Buy"? 1: -1)*_order->getLeavesQty();
    }
    _targetDelta = 0;
}


void Allocation::cancelDelta() {
    if (isNew()) {
        _size = 0.0;
        _order->setOrderQty(0.0);
        _order->setLeavesQty(0.0);
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
    }
}
