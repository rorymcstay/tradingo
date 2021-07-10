//
// Created by Rory McStay on 09/07/2021.
//
#include <gtest/gtest.h>
#include "Allocations.h"

TEST(Allocations, initialisation) {
    auto allocs =  std::make_shared<Allocations>(10, 0.5);
    ASSERT_EQ(allocs->allocations().size(), 2*(10/0.5));
    price_t price = 0;
    for (auto& alloc : *allocs) {
        ASSERT_EQ(price, alloc->getPrice());
        ASSERT_EQ(0, alloc->getSize());
        price += 0.5;
    }
    allocs->addAllocation(10.5, 100);
    ASSERT_EQ(allocs->totalAllocated(), 0);
    ASSERT_EQ(allocs->allocatedAtLevel(10.5), 0);
    ASSERT_EQ((*allocs)[10.5]->getTargetDelta(), 100);
    (*allocs)[10.5]->rest();
    ASSERT_EQ((*allocs)[10.5]->getSize(), 100);
    ASSERT_EQ((*allocs)[10.5]->getTargetDelta(), 0);
    ASSERT_EQ(allocs->totalAllocated(), 100);

    allocs->addAllocation(12, 100);
    allocs->addAllocation(12, -10);
    allocs->addAllocation(12, 20);
    allocs->restAll();
    auto res = allocs->allocatedAtLevel(12);
    ASSERT_EQ(res, 110);

}