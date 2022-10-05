
#include <gtest/gtest.h>
#include "Allocation.h"


TEST(Allocation, isNew) {

    auto alloc = Allocation(100, 0);
    alloc.setTargetDelta(100);

    ASSERT_EQ(alloc.getTargetDelta(), 100);
    ASSERT_EQ(alloc.getSize(), 0);
    ASSERT_TRUE(alloc.isNew());

    alloc.rest();

    ASSERT_TRUE(alloc.isNew());

    alloc.setTargetDelta(100);
    ASSERT_TRUE(alloc.isCancel());
    ASSERT_FALSE(alloc.isNew());
    ASSERT_FALSE(alloc.isAmendUp());
    ASSERT_FALSE(alloc.isAmendDown());

}
