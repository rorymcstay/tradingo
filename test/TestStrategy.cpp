//
// Created by Rory McStay on 09/07/2021.
//

#include <gtest/gtest.h>
#include "Strategy.h"
#include "fwk/TestOrdersApi.h"
#include "Allocations.h"
#include "fwk/TestEnv.h"


TEST(StrategyApi, smooke)
{
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"}
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);

    strategy->placeAllocations();
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy orderID=1" LN;
    strategy->allocations()->addAllocation(11,100.0);
    strategy->allocations()->addAllocation(9,-100.0);
    strategy->allocations()->addAllocation(10,-100.0);
    strategy->placeAllocations();
    // handle cancels first
    env >> "ORDER_CANCEL price=10 orderQty=0 symbol=XBTUSD side=Buy orderID=1 clOrdID=MCST0" LN;
    // TODO need to have handle on order created on line 28 to substitute.
    env >> "ORDER_NEW price=9 orderQty=100 side=Sell symbol=XBTUSD orderID=3 clOrdID=MCST3" LN;
    env >> "ORDER_NEW price=11 orderQty=100 side=Buy symbol=XBTUSD orderID=2 clOrdID=MCST4" LN;
    env >> "NONE" LN;
}

TEST(Strategy, changing_sides) {
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"}
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->placeAllocations();
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy orderID=1" LN;
    strategy->allocations()->addAllocation(10,-200);
    strategy->placeAllocations();
    env >> "ORDER_CANCEL price=10 orderQty=0 symbol=XBTUSD side=Buy orderID=1 clOrdID=MCST0" LN;
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Sell orderID=1" LN;
    env >> "NONE" LN;
}