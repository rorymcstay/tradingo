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
        {"referencePrice", "100"},
        {"shortTermWindow", "1"},
        {"longTermWindow", "2"},
        {"moving_average_crossover-interval", "10"},
        {"callback-signals", "true"},
        {"startingBalance", "10.0"}
    });

    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    auto order = env >> "ORDER_NEW Price=100 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    order->getOrdStatus();
    env >> "NONE" LN;
    env << "QUOTE askPrice=101.0 askSize=100.0 bidPrice=100.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> format("ORDER_CANCEL Price=100 OrderQty=0 CumQty=0 LeavesQty=0 OrdStatus=Canceled Side=Sell symbol=XBTUSD", order) + LN;
    auto order2 = env >> "ORDER_NEW Price=99 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Buy symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=101.0 askSize=100.0 bidPrice=100.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> format("ORDER_CANCEL Price=100 OrderQty=0 CumQty=0 LeavesQty=0 OrdStatus=Canceled Side=Buy symbol=XBTUSD", order2)+ LN;
    auto order3 = env >> "ORDER_NEW Price=101 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=99 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env >> format("ORDER_AMEND Price=0 LeavesQty=200 ClOrdID=MCST101.000000_v1 OrdStatus=Replaced", order3) + LN;
    env >> "ORDER_AMEND Price=0 OrderQty=0 CumQty=0 LeavesQty=200 OrderID=3 ClOrdID=MCST101.000000_v1 OrigClOrdID=MCST101.000000_v0 OrdStatus=Replaced Side= Symbol= " LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=38321 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env >> "NONE" LN;

}
