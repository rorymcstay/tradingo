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
    /// the underlying order on market for price level
    std::shared_ptr<model::Order> _order;
    /// the price level of allocation
    price_t _price;
    /// the size of the allocation at price level.
    qty_t _size;
    /// the desired change in size.
    qty_t _targetDelta;
    /// incremented in the case of an amend.
    int _version;

    /// reduce an allocations size by amount_
    void reduce(qty_t amount_) { _size -= amount_; }
public:
    /// the implied direction of the allocation
    std::string getSide() const;
    /// the implied new side of the allocation
    std::string targetSide() const { return (_size + _targetDelta < 0.0) ? "Sell" : "Buy"; }
    /// underlying order accesor
    const std::shared_ptr<model::Order> &getOrder() const { return _order; }
    /// order setter.
    void setOrder(const std::shared_ptr<model::Order> &order) { _order = order; }
    // accessors
    price_t getPrice() const { return _price; }
    void setPrice(price_t price) { _price = price; }
    qty_t getSize() const { return _size; }
    void setSize(qty_t size) { _size = size; }
    void setTargetDelta(qty_t delta_);
    qty_t getTargetDelta() const { return _targetDelta; }
    int getVersion() const { return _version; }

    /// declare an allocation reflected onto the exchange.
    void rest();
    /// cancel the change in size.
    void cancelDelta();
    /// if the targetDelta implies a change of sides. this means a cancel new.
    bool isChangingSide() const { return sgn(_targetDelta) != sgn(_size) && std::abs(_targetDelta) > std::abs(_size); }
    /// is a new order. This imples no underlying order
    bool isNew() const { return almost_equal(_size, 0.0) && !almost_equal(_targetDelta, 0.0); }
    /// is an increase in allocation size.
    bool isAmendUp() const { return !isCancel() && !isChangingSide() && std::abs(_targetDelta+_size) > std::abs(_size); }
    /// is a decrease in allocation size.
    bool isAmendDown() const {return !isCancel() && !isChangingSide() && std::abs(_targetDelta+_size) < std::abs(_size); };
    /// is an exit of allocation.
    bool isCancel() const { return almost_equal(_targetDelta + _size, 0.0); };
    void update(const std::shared_ptr<model::Execution>& exec_);


public:
    Allocation();
    Allocation(price_t price_, size_t qty_);

};


#endif //MY_PROJECT_ALLOCATION_H
