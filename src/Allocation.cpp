//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"
#include "Utils.h"

Allocation::Allocation()
:   _price(0.0)
,   _size(0.0)
,   _order(nullptr) {

}

Allocation::Allocation(price_t price_, size_t qty_)
:   _price(price_)
,   _size(0)
,   _targetDelta(qty_)
,   _order(nullptr) {

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
