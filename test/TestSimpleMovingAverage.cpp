//
// Created by Rory McStay on 08/07/2021.
//

#include <gtest/gtest.h>
#include "SimpleMovingAverage.h"

TEST(SimpleMovingAverageTest, smoke_test) {
    auto sma = SimpleMovingAverage<double,double>(10, 10);

    sma(5);
    ASSERT_FALSE(sma.is_ready());
    sma(5);
    sma(5);
    sma(5);
    sma(5);
    ASSERT_FALSE(sma.is_ready());
    sma(5);
    sma(5);
    sma(5);
    std::cout << sma(5) << "\n";
    ASSERT_EQ(sma(5), 5);
    ASSERT_TRUE(sma.is_ready());

}