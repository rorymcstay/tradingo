//
// Created by Rory McStay on 09/07/2021.
//
#include <gtest/gtest.h>
#include <memory>
#include "Allocations.h"
#include "fwk/TestOrdersApi.h"

TEST(Allocations, initialisation) {
    auto oapi = std::make_shared<TestOrdersApi>(nullptr);
    auto allocs =  std::make_shared<Allocations<TestOrdersApi>>(
        oapi, "XBTUSD", "MCST", 0,
        10, 0.5, 100);
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
    ASSERT_EQ(allocs->get(10.5)->getTargetDelta(), 100);
    allocs->get(10.5)->getOrder()->setLeavesQty(100);
    allocs->get(10.5)->rest();
    ASSERT_EQ(allocs->get(10.5)->getSize(), 100);
    ASSERT_EQ(allocs->get(10.5)->getTargetDelta(), 0);
    ASSERT_EQ(allocs->totalAllocated(), 100);

    allocs->addAllocation(12, 100);
    allocs->addAllocation(12, -10);
    allocs->addAllocation(12, 20);
    allocs->get(10.5)->getOrder()->setLeavesQty(110);
    allocs->get(12)->rest();
    auto res = allocs->allocatedAtLevel(12);
    ASSERT_EQ(res, 110);

}


TEST(Allocations, test_placing_new) {
    auto mdi = std::make_shared<TestMarketData>();
    mdi->init();
    auto oapi = std::make_shared<TestOrdersApi>(nullptr);
    oapi->setMarketData(mdi);
    auto allocs =  std::make_shared<Allocations<TestOrdersApi>>(
        oapi, "XBTUSD", "MCST", 0,
        10, 0.5, 100);
    allocs->addAllocation(12, 100);
    allocs->placeAllocations();
    allocs->cancelOrders([](const std::shared_ptr<Allocation>& alloc_) { return true; });


}




TEST(Allocations, test_iteration_over_allocations) {

    auto oapi = std::make_shared<TestOrdersApi>(nullptr);
    auto allocs =  std::make_shared<Allocations<TestOrdersApi>>(
        oapi, "XBTUSD", "MCST", 0,
        10, 0.5, 100);
    ASSERT_EQ(allocs->allocations().size(), 2*(10/0.5));
    price_t price = 0;
    for (auto& alloc : *allocs) {
        ASSERT_EQ(price, alloc->getPrice());
        ASSERT_EQ(0, alloc->getSize());
        price += 0.5;
    }

    int count = 0;
    for (auto& alloc : *allocs) {
        count++;
    }
    ASSERT_EQ(count, 0);
    allocs->addAllocation(12, 100);
    allocs->addAllocation(13, -10);
    allocs->addAllocation(14, 20);
    for (auto& alloc : *allocs) {
        count++;
    }
    ASSERT_EQ(count, 3);
    allocs->addAllocation(12, 100);
    for (auto& alloc : *allocs) {
        count++;
    }
    ASSERT_EQ(count, 6);
}

TEST(Allocation, isNew) {

    auto alloc = Allocation(100, 0);
    alloc.setTargetDelta(100);

    ASSERT_EQ(alloc.getTargetDelta(), 100);
    ASSERT_EQ(alloc.getSize(), 0);
    ASSERT_TRUE(alloc.isNew());
    alloc.getOrder()->setLeavesQty(100);
    alloc.getOrder()->setSide("Buy");

    alloc.rest();
    ASSERT_EQ(alloc.getSize(), 100);

    ASSERT_FALSE(alloc.isNew());
    ASSERT_FALSE(alloc.isCancel());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_FALSE(alloc.isChangingSide());

    alloc.setTargetDelta(-100);
    ASSERT_TRUE(alloc.isCancel());
    ASSERT_FALSE(alloc.isNew());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_FALSE(alloc.isChangingSide());
    alloc.getOrder()->setOrdStatus("Canceled");
    alloc.rest();

    ASSERT_FALSE(alloc.isCancel());
    ASSERT_FALSE(alloc.isNew());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_FALSE(alloc.isChangingSide());
    ASSERT_EQ(alloc.getSize(), 0);

    alloc.setTargetDelta(-100);
    ASSERT_FALSE(alloc.isCancel());
    ASSERT_TRUE(alloc.isNew());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_FALSE(alloc.isChangingSide());
    alloc.getOrder()->setLeavesQty(100);
    alloc.getOrder()->setOrdStatus("New");
    alloc.getOrder()->setSide("Sell");
    alloc.rest();

    alloc.setTargetDelta(-100);
    ASSERT_FALSE(alloc.isCancel());
    ASSERT_FALSE(alloc.isNew());
    ASSERT_TRUE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_FALSE(alloc.isChangingSide());
    alloc.rest();

    alloc.setTargetDelta(300);
    ASSERT_FALSE(alloc.isCancel());
    ASSERT_FALSE(alloc.isNew());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());
    ASSERT_TRUE(alloc.isChangingSide());
    alloc.getOrder()->setSide("Buy");
    alloc.getOrder()->setLeavesQty(100.0);
    alloc.rest();

}
