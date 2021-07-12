//
// Created by rory on 16/06/2021.
//
#include <gtest/gtest.h>
#define _TURN_OFF_PLATFORM_STRING
#include "fwk/TestEnv.h"

TEST(BreakOutStrategy, smoke_test)
{
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "1"},
        {"longTermWindow", "2"}
    });

    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE askPrice=101.0 askSize=100.0 bidPrice=100.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW ordStatus=New orderQty=100 price=100 side=Buy symbol=XBTUSD orderID=1 clOrdID=MCST0 " LN;
    env >> "NONE" LN;
    // env >> "ORDER_NEW price=99.0 side=Buy orderQty=100.0 symbol=XBTUSD" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=99 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=38321 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;

}

TEST(BreakOutStrategy, DISABLED_test_playback) {
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "35000"},
        {"shortTermWindow", "10"},
        {"longTermWindow", "100"},
        {"logLevel", "info"}
    });
    env.playback("trades_XBTUSD.json", "quotes_XBTUSD.json");
}