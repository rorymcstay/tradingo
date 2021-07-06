//
// Created by rory on 16/06/2021.
//
#include <gtest/gtest.h>
#define _TURN_OFF_PLATFORM_STRING
#include "fwk/TestEnv.h"

TEST(BreakOutStrategy, creating_positions)
{
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"}
    });

    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW price=99.0 side=Buy orderQty=500.0 symbol=XBTUSD" LN;
    env << "TRADE price=100.0 size=1000 symbol=XBTUSD" LN;
}