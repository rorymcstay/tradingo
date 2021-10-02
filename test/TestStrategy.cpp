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
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->placeAllocations();
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy orderID=1" LN;
    strategy->allocations()->addAllocation(11,100.0);
    strategy->allocations()->addAllocation(9,-100.0);
    strategy->allocations()->addAllocation(10,-100.0);
    strategy->placeAllocations();
    env >> "ORDER_NEW Price=9 OrderQty=100 CumQty=0 LeavesQty=100 OrderID=4 ClOrdID=MCST1 OrigClOrdID= OrdStatus=New Side=Sell symbol=XBTUSD" LN;
    env >> "ORDER_CANCEL Price=10 OrderQty=0 CumQty=0 LeavesQty=0 OrderID=6 ClOrdID=MCST3 OrigClOrdID= OrdStatus=New Side=Buy symbol=XBTUSD" LN;
    env >> "ORDER_NEW price=11 orderQty=100 side=Buy symbol=XBTUSD orderID=3 clOrdID=MCST3" LN;
    env >> "NONE" LN;
}

TEST(Strategy, changing_sides) {
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->placeAllocations();
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy orderID=1" LN;
    strategy->allocations()->addAllocation(10,-200);
    strategy->placeAllocations();
    env >> "ORDER_NEW Price=10 OrderQty=100 CumQty=0 LeavesQty=100 OrderID=4 ClOrdID=MCST1 OrigClOrdID= OrdStatus=New Side=Sell Symbol=XBTUSD" LN;
    env >> "ORDER_CANCEL Price=10 OrderQty=0 CumQty=0 LeavesQty=0 OrderID=2 ClOrdID=MCST0 OrigClOrdID= OrdStatus=Canceled Side=Buy Symbol=XBTUSD" LN;
    env >> "NONE" LN;
}

TEST(Strategy, amend_order_more_than_once)
{
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
        {"referencePrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
});

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->placeAllocations();
    env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy orderID=1 clOrdID=MCST0" LN;
    strategy->allocations()->addAllocation(10,100);
    strategy->placeAllocations();
    env >> "ORDER_AMEND price=10 orderQty=200 symbol=XBTUSD  side=Buy clOrdID=MCST1" LN;
    strategy->allocations()->addAllocation(10,100);
    strategy->placeAllocations();
    env >> "ORDER_AMEND price=10 orderQty=300 symbol=XBTUSD  side=Buy clOrdID=MCST2" LN;
    strategy->allocations()->addAllocation(10,100);
    strategy->placeAllocations();
    env >> "ORDER_AMEND price=10 orderQty=400 symbol=XBTUSD  side=Buy clOrdID=MCST3" LN;
}

TEST(Strategy, test_time_control) {
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"startingAmount", "1000"},
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