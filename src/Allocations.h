//
// Created by rory on 08/07/2021.
//

#ifndef MY_PROJECT_ALLOCATIONS_H
#define MY_PROJECT_ALLOCATIONS_H
#include "Allocation.h"

class Allocations {

private:
    std::vector<std::shared_ptr<Allocation>> _data;
    price_t _tickSize;
    price_t _referencePrice;
    price_t _lowPrice;
    price_t _highPrice;
    qty_t _lotSize;

    bool _modified;
public:
    size_t allocIndex(price_t price_);
    Allocations(price_t midPoint_, price_t tickSize_, qty_t lotSize_);
    /// add to an allocation by price level.
    void addAllocation(price_t price_, qty_t qty_, const std::string& side_="");
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
    std::vector<std::shared_ptr<Allocation>>::iterator begin() { return _data.begin(); }//*_data[_lowPrice]; }
    std::vector<std::shared_ptr<Allocation>>::iterator end() { return  _data.end(); }
    /// declare all allocations fulfilled.
    void restAll();
    /// declare allocations fulfilled
    void rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_);
    /// reset the desired allocation delta.
    void cancel(const std::function<bool(const std::shared_ptr<Allocation>& )>& predicate_);
    /// cancel all price levels
    void cancelOrders(const std::function<bool(const std::shared_ptr<Allocation> &)> &predicate_);
};



#endif //MY_PROJECT_ALLOCATIONS_H
