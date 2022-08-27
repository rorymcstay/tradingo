//
// Created by rory on 16/06/2021.
//
#include <gtest/gtest.h>
#define _TURN_OFF_PLATFORM_STRING
#include "../fwk/TestEnv.h"

TEST(BreakOutStrategy, smoke_test)
{
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "40000"},
        {"shortTermWindow", "0.000000"},
        {"longTermWindow", "0.0000"},
        {"moving_average_crossover-interval", "10"},
        {"callback-signals", "true"},
    });

    env << "QUOTE askPrice=40001.0 askSize=40001.0 bidPrice=40000.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=40001.0 askSize=40001.0 bidPrice=40000.0 bidSize=1000.0 symbol=XBTUSD" LN;
    auto order = env >> "ORDER_NEW Price=40001 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=40002.0 askSize=40001.0 bidPrice=40001.0 bidSize=1000.0 symbol=XBTUSD" LN;
    auto order2 = env >> format("ORDER_AMEND Price=40001.0 OrderQty=200 LeavesQty=200 Side=Buy Symbol=XBTUSD", order) + LN;
    //env >> format("ORDER_CANCEL Price=100 OrderQty=200 OrdStatus=Canceled Side=Sell Symbol=XBTUSD ", order) + LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=40004.0 askSize=40001.0 bidPrice=40003.0 bidSize=1000.0 symbol=XBTUSD" LN;
    auto order5 = env >> "ORDER_NEW Price=40003 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Buy Symbol=XBTUSD " LN;
    env >> "NONE" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=40000 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=39999 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env >> "NONE" LN;

}
