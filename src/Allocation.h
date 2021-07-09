//
// Created by Rory McStay on 07/07/2021.
//

#ifndef MY_PROJECT_ALLOCATION_H
#define MY_PROJECT_ALLOCATION_H

#include <model/Order.h>
#include <model/Execution.h>
#include "Utils.h"


using price_t = double;
using qty_t = double;
using namespace io::swagger::client;

class Allocation {
    std::shared_ptr<model::Order> _order;
    price_t _price;
    qty_t _size;
    qty_t _targetDelta;
public:
    std::string getSide() const { return (_size < 0) ? "Sell" : "Buy"; }
    const std::shared_ptr<model::Order> &getOrder() const { return _order; }
    void setOrder(const std::shared_ptr<model::Order> &order) { _order = order; }
    price_t getPrice() const { return _price; }
    void setPrice(price_t price) { _price = price; }
    qty_t getSize() const { return _size; }
    void setSize(qty_t size) { _size = size; }
    void setTargetDelta(qty_t delta_) { _targetDelta = delta_; }
    qty_t getTargetDelta() const { return _targetDelta; }

    void rest();
    void cancelDelta();
    bool isChangingSide() const { return sgn(_targetDelta) != sgn(_size) && std::abs(_targetDelta) >= std::abs(_size); }
    bool isNew() const { return _size == 0 && _targetDelta != 0; }
    bool isAmendUp() const { return !isChangingSide() && std::abs(_targetDelta+_size) > _targetDelta+_size; }
    bool isAmendDown() const {return !isChangingSide() && std::abs(_targetDelta+_size) < _targetDelta+_size; };
    bool isCancel() const { return _targetDelta == _size; };

public:
    Allocation();
    Allocation(price_t price_, size_t qty_);

};


#endif //MY_PROJECT_ALLOCATION_H