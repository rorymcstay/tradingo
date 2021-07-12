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
    void addAllocation(price_t price_, qty_t qty_, const std::string& side_="");
    std::vector<std::shared_ptr<Allocation>> allocations() { return _data; }

    price_t roundTickPassive(price_t price_);
    qty_t roundLotSize(qty_t size_);
    qty_t allocatedAtLevel(price_t price_);
    qty_t totalAllocated();

    const std::shared_ptr<Allocation>& get(price_t price_);
    void update(const std::shared_ptr<model::Execution>& exec_);
    bool modified() { return _modified; }
    void setUnmodified() { _modified = false; }

    const std::shared_ptr<Allocation>& operator[] (price_t price_) { return get(price_); }

    std::vector<std::shared_ptr<Allocation>>::iterator begin() { return _data.begin(); }//*_data[_lowPrice]; }
    std::vector<std::shared_ptr<Allocation>>::iterator end() { return  _data.end(); }

    void restAll();
    void rest(const std::function<bool(const std::shared_ptr<Allocation>&)>& predicate_);
    void cancel(const std::function<bool(const std::shared_ptr<Allocation>& )>& predicate_);

};



#endif //MY_PROJECT_ALLOCATIONS_H
