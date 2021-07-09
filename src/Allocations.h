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

    bool _modified;
public:
    size_t allocIndex(price_t price_);
    Allocations(price_t midPoint_, price_t tickSize_);
    void addAllocation(price_t price_, size_t qty_, const std::string& side_);
    std::vector<std::shared_ptr<Allocation>> allocations() { return _data; }

    price_t roundTickPassive(price_t price_);;

    qty_t allocatedAtLevel(price_t price_);
    qty_t totalAllocated();

    const std::shared_ptr<Allocation>& get(price_t price_);
    void update(const std::shared_ptr<model::Execution>& exec_);
    bool modified() { return _modified; }

    const std::shared_ptr<Allocation>& operator[] (price_t price_) { return get(price_); }

    std::vector<std::shared_ptr<Allocation>>::iterator begin() { return _data.begin(); }
    std::vector<std::shared_ptr<Allocation>>::iterator end() { return _data.end(); }

};



#endif //MY_PROJECT_ALLOCATIONS_H
