//
// Created by Rory McStay on 09/07/2021.
//

#include <gtest/gtest.h>
#include "Strategy.h"
#include "fwk/TestOrdersApi.h"
#include "fwk/TestEnv.h"
#include "Utils.h"


TEST(StrategyApi, smooke)
{
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", "1000"}
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->allocations()->placeAllocations();
    auto order = env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD Side=Buy LeavesQty=100" LN;
    strategy->allocations()->addAllocation(11,100.0);
    strategy->allocations()->addAllocation(9,-100.0);
    strategy->allocations()->addAllocation(10,-100.0);
    strategy->allocations()->placeAllocations();
    auto order2 = env >> "ORDER_NEW Price=9 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> format("ORDER_CANCEL Price=10 OrderQty=0 CumQty=0 LeavesQty=0 OrdStatus=New Side=Buy symbol=XBTUSD", order);
    auto order3 = env >> "ORDER_NEW Price=11 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Buy symbol=XBTUSD" LN;
    env >> "NONE" LN;
}

TEST(Strategy, changing_sides) {
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
    });

    auto allocations = env.strategy()->allocations();
    allocations->addAllocation(10, 100.0);
    allocations->placeAllocations();
    auto order = env >> "ORDER_NEW Price=10 OrderQty=100 Symbol=XBTUSD Side=Buy LeavesQty=100" LN;
    allocations->addAllocation(10,-200);
    allocations->placeAllocations();
    auto order2 = env >> "ORDER_NEW Price=10 OrderQty=100 CumQty=0 LeavesQty=100 OrdStatus=New Side=Sell Symbol=XBTUSD" LN;
    order = env >> format("ORDER_CANCEL Price=10 OrderQty=0 CumQty=0 LeavesQty=0 OrdStatus=Canceled Side=Buy Symbol=XBTUSD", order) + LN;
    env >> "NONE" LN;
}

TEST(Strategy, amend_order_more_than_once)
{
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", "10"},
    });

    auto allocations = env.strategy()->allocations();
    allocations->addAllocation(10, 100.0);
    allocations->placeAllocations();
    auto order = env >> "ORDER_NEW Price=10 OrderQty=100 Symbol=XBTUSD  Side=Buy LeavesQty=100" LN;
    allocations->addAllocation(10,100.0);
    allocations->placeAllocations();
    auto order2 = env >> format("ORDER_AMEND Price=10 OrderQty=200 Symbol=XBTUSD  Side=Buy LeavesQty=200", order) + LN;
    allocations->addAllocation(10,100.0);
    allocations->placeAllocations();
    auto order3 = env >> format("ORDER_AMEND Price=10 OrderQty=300 Symbol=XBTUSD  Side=Buy LeavesQty=300", order2) + LN;
    allocations->addAllocation(10,100.0);
    allocations->placeAllocations();
    env >> format("ORDER_AMEND Price=10 OrderQty=400 Symbol=XBTUSD  Side=Buy LeavesQty=400", order3) + LN;
}


TEST(Strategy, test_time_control) {
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"shortTermWindow", "5"},
        {"longTermWindow", "10"},
        {"realtime", "true"},
});

    auto timebegin = utility::datetime::utc_now();
    env << "QUOTE timestamp=2021-07-09T01:38:24.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:25.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:26.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:27.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:28.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:29.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:30.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:31.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:32.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:33.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:34.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    env << "QUOTE timestamp=2021-07-09T01:38:35.992Z askPrice=100.0 askSize=100.0 bidPrice=99.0 bidSize=1000.0 symbol=XBTUSD" LN;
    auto timeend = utility::datetime::utc_now();
    LOGINFO(LOG_NVP("ElapsedTime", timeend - timebegin));
    ASSERT_EQ(timeend - timebegin,11);

}

TEST(Strategy, time_diff) {
    auto time = utility::datetime::from_string("2021-07-09T01:38:28.992000Z", utility::datetime::ISO_8601);
    auto time_p1 = utility::datetime::from_string("2021-07-09T01:38:28.999000Z", utility::datetime::ISO_8601);
    LOGINFO(LOG_VAR(time.to_string()) << LOG_VAR(time_p1.to_string()));
    double diff = time_diff(time_p1, time, "milliseconds");
    LOGINFO(LOG_NVP("TimeDiff", diff));
    ASSERT_EQ(diff, 7);
}

TEST(Strategy, balance_is_updated_during_test) {

    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"fairPrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", "0.1"}
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->allocations()->placeAllocations();
    auto order = env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy leavesQty=100" LN;
    env >> "NONE" LN;
    env << format("EXECUTION side=Buy lastPx=9.99 lastQty=100 execType=Trade", order) + LN;
    auto position = env.strategy()->getMD()->getPositions().at("XBTUSD");
    ASSERT_DOUBLE_EQ(position->getCurrentCost(), 10.01001001001001);
    ASSERT_DOUBLE_EQ(position->getCurrentQty(), 100);
    ASSERT_DOUBLE_EQ(position->getBankruptPrice(), 0.0);
    ASSERT_DOUBLE_EQ(position->getBreakEvenPrice(), 9.99);
    ASSERT_DOUBLE_EQ(position->getLiquidationPrice(), 0.23333333333332984);
    auto margin = env.strategy()->getMD()->getMargin();
    ASSERT_DOUBLE_EQ(margin->getWalletBalance(), 0.00010000000000000286);
    ASSERT_DOUBLE_EQ(margin->getMaintMargin(), 0.035000000000000003);
    ASSERT_DOUBLE_EQ(margin->getAvailableMargin(), -0.0349);
    ASSERT_DOUBLE_EQ(margin->getUnrealisedPnl(), 0.0);
}
