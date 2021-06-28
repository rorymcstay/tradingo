//
// Created by rory on 16/06/2021.
//
#include <gtest/gtest.h>
#define _TURN_OFF_PLATFORM_STRING
#include "fwk/TestEnv.h"

TEST(TestBitMexAPI, creating_positions)
{
    TestEnv env("/path/to/config.xml");
    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW Price=99.5 Side=Buy OrdQty=10" LN;
}