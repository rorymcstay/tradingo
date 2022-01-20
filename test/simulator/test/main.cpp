#include <gtest/gtest.h>

#include "fwk/TestEnv.cpp"
#include "fwk/TestDomain.h"

using TestOrderBook = OrderBook<TestOrder>;
using TestEnvImpl = TestEnv<TestOrder>;

TEST(TestOrderBook, smoke_test)
{
    TestEnvImpl env("XYZ", 1.0);

    env << "NewOrder Price=1.0 OrdQty=100 Side=Buy TraderID=1" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 1.0), 100);
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.0 Side=Buy OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID=1" LN;
    env << "NewOrder Price=1.0 OrdQty=100 Side=Sell TraderID=2" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.0 Side=Sell OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID=2" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=Filled Price=1.0 OrdQty=100 LastQty=100 CumQty=100 LastPrice=1.0 OrderID=2" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=Filled Price=1.0 OrdQty=100 LastQty=100 CumQty=100 LastPrice=1.0 OrderID=1" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 1.0), 0);
    env >> "NONE" LN;
}

TEST(TestOrderBook, partial_fill)
{
    TestEnvImpl env("XYZ", 1.0);

    env << "NewOrder Price=1.0 OrdQty=100 Side=Buy TraderID=1" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.0 Side=Buy OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID=1" LN;
    env << "NewOrder Price=1.0 OrdQty=50 Side=Sell TraderID=2" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.0 Side=Sell OrdQty=50 LastQty=0 CumQty=0 LastPrice=0 OrderID=2" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=Filled Price=1.0 OrdQty=50 LastQty=50 CumQty=50 LastPrice=1.0 OrderID=2" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=PartiallyFilled Price=1.0 Side=Sell OrdQty=100 LastQty=50 CumQty=50 LastPrice=1.0 OrderID=1" LN;
}

TEST(TestOrderBook, multiple_levels)
{
    TestEnvImpl env("XYZ", 1.0);
    env << "NewOrder Price=1.0 OrdQty=100 Side=Buy TraderID=1" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.0 Side=Buy OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID=1" LN;
    env << "NewOrder Price=1.1 OrdQty=50 Side=Sell TraderID=2" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=1.1 Side=Sell OrdQty=50 LastQty=0 CumQty=0 LastPrice=0 OrderID=2" LN;
    env << "NewOrder Price=1.2 OrdQty=50 Side=Sell TraderID=3" LN;
    env << "NewOrder Price=0.9 OrdQty=50 Side=Buy TraderID=4" LN;
    env << "NewOrder Price=1.0 OrdQty=50 Side=Buy TraderID=4" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 0.9), 50);
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 1.0), 150);
    ASSERT_EQ(env.orderBook()->bestAsk(), 1.1);
    ASSERT_EQ(env.orderBook()->bestBid(), 1.0);
}

TEST(TestOrderBook, multiple_levels_close_price_greater_than_10)
{
    TestEnvImpl env("XYZ", 50.32);
    env << "NewOrder Price=50.0 OrdQty=100 Side=Buy TraderID=1" LN;
    env << "NewOrder Price=51.0 OrdQty=50 Side=Sell TraderID=2" LN;
    env << "NewOrder Price=52.0 OrdQty=50 Side=Sell TraderID=3" LN;
    env << "NewOrder Price=49.0 OrdQty=50 Side=Buy TraderID=4" LN;
    env << "NewOrder Price=50.0 OrdQty=50 Side=Buy TraderID=4" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 49), 50);
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 50), 150);
    ASSERT_EQ(env.orderBook()->bestAsk(), 51);
    ASSERT_EQ(env.orderBook()->bestBid(), 50);
}

TEST(TestOrderBook, trader_cannot_exceed_100_messages_per_second)
{
    TestEnvImpl env("XYZ", 50.32);
    for (int i(0); i < 100;)
    {
        env << "NewOrder Price=50.0 OrdQty=100 Side=Buy TraderID=1" LN;
        env >> ("ExecReport ExecType=New OrdStatus=New Price=50.0 Side=Buy OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID="+ std::to_string(++i) + "" LN);
    }
    env << "NewOrder Price=50.0 OrdQty=100 Side=Buy TraderID=1" LN;
    env >> "ExecReport OrdStatus=Rejected ExecType=Reject OrderID=101 OrdQty=100 Side=Buy LastQty=0 CumQty=0 LastPrice=0 Price=50.0 Text=Message_rate_exceeded" LN;
}

TEST(TestOrderBook, trader_can_modify_quantity_down_on_order)
{
    TestEnvImpl env("XYZ", 50.32);
    env << "NewOrder Price=50.0 OrdQty=100 Side=Buy TraderID=1" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.0 Side=Buy OrdQty=100 LastQty=0 CumQty=0 LastPrice=0 OrderID=1" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 50), 100);
    env << "CancelOrder OrdQty=90 OrderID=1 Price=50.0 Side=Buy TraderID=1" LN;
    env >> "ExecReport ExecType=Replaced OrdQty=90 OrderID=1 Price=50.0 OrdStatus=New LastQty=0 CumQty=0 OrderID=1" LN;
    ASSERT_EQ(env.orderBook()->qtyAtLevel(Side::Buy, 50), 90);
    env >> "NONE" LN;
}

TEST(TestOrderBook, alcova_example)
{
    TestEnvImpl env("XYZ", 50.32);
    env << "NewOrder Price=50.34 OrdQty=30 Side=Sell TraderID=1" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.34 Side=Sell OrdQty=30 LastQty=0 CumQty=0 LastPrice=0 OrderID=1" LN;
    env << "NewOrder Price=50.32 OrdQty=20 Side=Sell TraderID=2" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.32 Side=Sell OrdQty=20 LastQty=0 CumQty=0 LastPrice=0 OrderID=2" LN;
    env << "NewOrder Price=50.31 OrdQty=20 Side=Buy TraderID=4" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.31 Side=Buy OrdQty=20 LastQty=0 CumQty=0 LastPrice=0 OrderID=3" LN;
    env << "NewOrder Price=50.31 OrdQty=40 Side=Buy TraderID=3" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.31 Side=Buy OrdQty=40 LastQty=0 CumQty=0 LastPrice=0 OrderID=4" LN;
    env << "NewOrder Price=50.30 OrdQty=10 Side=Buy TraderID=5" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.30 Side=Buy OrdQty=10 LastQty=0 CumQty=0 LastPrice=0 OrderID=5" LN;
    env << "NewOrder Price=50.30 OrdQty=15 Side=Buy TraderID=6" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.30 Side=Buy OrdQty=15 LastQty=0 CumQty=0 LastPrice=0 OrderID=6" LN;
    env >> "NONE" LN;

    // sell order for 25 shares
    env << "NewOrder Side=Sell Price=50.31 OrdQty=25 TraderID=7" LN;
    env >> "ExecReport ExecType=New OrdStatus=New Price=50.31 Side=Sell OrdQty=25 LastQty=0 CumQty=0 LastPrice=0 OrderID=7" LN;
    // 4 trades come out, 2 matches occur
    env >> "ExecReport ExecType=Trade OrdStatus=PartiallyFilled Price=50.31 Side=Sell OrdQty=25 CumQty=20 LastPrice=50.31 LastQty=20 OrderID=7" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=Filled Price=50.31 Side=Buy OrdQty=20 CumQty=20 LastPrice=50.31 LastQty=20 OrderID=3" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=Filled Price=50.31 Side=Sell OrdQty=25 CumQty=25 LastPrice=50.31 LastQty=5 OrderID=7" LN;
    env >> "ExecReport ExecType=Trade OrdStatus=PartiallyFilled Price=50.31 Side=Buy OrdQty=40 CumQty=5 LastPrice=50.31 LastQty=5 OrderID=4" LN;
    env >> "NONE" LN;

}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
