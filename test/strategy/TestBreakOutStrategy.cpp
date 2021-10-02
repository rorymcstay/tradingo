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
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "1"},
        {"longTermWindow", "2"},
        {"moving_average_crossover-interval", "10"},
        {"callback-signals", "true"}
    });

    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW Price=100 OrderQty=100 CumQty=0 LeavesQty=100 OrderID=2 ClOrdID=MCST0 OrigClOrdID= OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=101.0 askSize=100.0 bidPrice=100.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW Price=100 OrderQty=100 CumQty=0 LeavesQty=100 OrderID=4 ClOrdID=MCST1 OrigClOrdID= OrdStatus=New Side=Buy symbol=XBTUSD" LN;
    env >> "ORDER_CANCEL Price=100 OrderQty=0 CumQty=0 LeavesQty=0 OrderID=2 ClOrdID=MCST0 OrigClOrdID= OrdStatus=Canceled Side=Sell symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "QUOTE askPrice=101.0 askSize=100.0 bidPrice=100.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env >> "ORDER_NEW Price=101 OrderQty=100 CumQty=0 LeavesQty=100 OrderID=6 ClOrdID=MCST3 OrigClOrdID= OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> "ORDER_CANCEL Price=100 OrderQty=0 CumQty=0 LeavesQty=0 OrderID=4 ClOrdID=MCST1 OrigClOrdID= OrdStatus=Canceled Side=Buy symbol=XBTUSD" LN;
    env >> "NONE" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=99 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env >> "NONE" LN;
    env << "TRADE foreignNotional=500 grossValue=1304800 homeNotional=0.013048000000000001 price=38321 side=Sell size=500 symbol=XBTUSD tickDirection=ZeroMinusTick trdMatchID=dac0793c-0ff4-74f5-a793-b00e3811a690" LN;
    env >> "NONE" LN;

}