//
// Created by Rory McStay on 09/07/2021.
//

#include <gtest/gtest.h>
#include "Functional.h"
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
        {"startingBalance", "10"},
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

    price_t startingBalance = 10.0;

    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"fairPrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", std::to_string(startingBalance)}
    });

    auto strategy = env.strategy();
    strategy->allocations()->addAllocation(10, 100.0);
    strategy->allocations()->placeAllocations();
    auto order = env >> "ORDER_NEW price=10 orderQty=100 symbol=XBTUSD  side=Buy leavesQty=100" LN;
    env >> "NONE" LN;
    env << format("EXECUTION side=Buy lastPx=9.99 lastQty=100 execType=Trade", order) + LN;
    auto position = env.strategy()->getMD()->getPositions().at("XBTUSD");
    auto expectedCurrentCost = 0.66733400066733406;
    ASSERT_DOUBLE_EQ(position->getCurrentCost(), expectedCurrentCost);
    ASSERT_DOUBLE_EQ(position->getCurrentQty(), 100);
    // the bankruptcy price is invalid, as our leverage is > 1, so we will go bankrupt before price goes to zero
    ASSERT_DOUBLE_EQ(position->getBankruptPrice(), 0.0);
    ASSERT_DOUBLE_EQ(position->getBreakEvenPrice(), 9.99);
    ASSERT_DOUBLE_EQ(position->getLiquidationPrice(), 0.23333333333332984);
    auto margin = env.strategy()->getMD()->getMargin();
    ASSERT_DOUBLE_EQ(margin->getWalletBalance(), 9.3333333333333339); // ~= startingBalance - currentCost
    ASSERT_DOUBLE_EQ(margin->getMaintMargin(), 0.035);
    ASSERT_DOUBLE_EQ(margin->getAvailableMargin(), 9.2983333333333338);
    ASSERT_DOUBLE_EQ(margin->getUnrealisedPnl(), 0.0);
}


TEST(Strategy, position_margin_is_updated_during_test) {

    price_t startingBalance = 0.003521;

    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "100"},
        {"fairPrice", "100"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", std::to_string(startingBalance)}
    });
    env << "MARGIN Account=365570.0 Currency=XBt RiskLimit=1000000000000.0 Amount=351951.0 PrevRealisedPnl=-1959.0 GrossComm=-290.0 GrossMarkValue=1333800.0 RiskValue=1333800.0 MaintMargin=131305.0 RealisedPnl=88.0 UnrealisedPnl=-5588.0 WalletBalance=352039.0 MarginBalance=346451.0 MarginBalancePcnt=0.2597 MarginLeverage=3.8498950789577746 MarginUsedPcnt=0.379 ExcessMargin=215146.0 ExcessMarginPcnt=0.1613 AvailableMargin=215146.0 WithdrawableMargin=215146.0 Timestamp=2022-01-30 19:50:55.814000+00:00 GrossLastValue=1333800.0";
    env << "POSITION Account=365570.0 Symbol=XBTUSD Currency=XBt Underlying=XBT QuoteCurrency=USD Commission=0.0005 InitMarginReq=0.1 MaintMarginReq=0.0035 RiskLimit=20000000000.0 Leverage=10.0 DeleveragePercentile=0.4 PrevRealisedPnl=-1959.0 PrevClosePrice=37530.93 OpeningTimestamp=2022-01-30 20:00:00+00:00 OpeningQty=500.0 OpeningCost=-1328010.0 OpeningComm=-290.0 OpenOrderBuyQty=400.0 OpenOrderBuyCost=-1071848.0 ExecBuyQty=700.0 ExecBuyCost=1870472.0 ExecSellQty=400.0 ExecSellCost=1062120.0 ExecQty=300.0 ExecCost=-808352.0 ExecComm=-5805.0 CurrentTimestamp=2022-01-30 20:13:30.814000+00:00 CurrentQty=800.0 CurrentCost=-2136362.0 CurrentComm=-6095.0 RealisedCost=-248.0 UnrealisedCost=-2136114.0 GrossOpenCost=1071848.0 GrossExecCost=801631.0 IsOpen=True MarkPrice=37363.36 MarkValue=-2141136.0 RiskValue=3212984.0 HomeNotional=0.02141136 ForeignNotional=-800.0 PosCost=-2136114.0 PosCost2=-2136114.0 PosCross=2998.0 PosInit=213612.0 PosComm=1177.0 PosMargin=217787.0 PosMaint=8654.0 InitMargin=108311.0 MaintMargin=212765.0 RealisedGrossPnl=248.0 RealisedPnl=6343.0 UnrealisedGrossPnl=-5022.0 UnrealisedPnl=-5022.0 UnrealisedPnlPcnt=-0.0024 UnrealisedRoePcnt=-0.0235 AvgCostPrice=37451.2198 AvgEntryPrice=37451.2198 BreakEvenPrice=37340.5 MarginCallPrice=34112.0 LiquidationPrice=34112.0 BankruptPrice=34003.5 Timestamp=2022-01-30 20:13:30.814000+00:00 LastPrice=37363.36 LastValue=-2141136.0";
    env << "INSTRUMENT Symbol=XBTUSD RootSymbol=XBT State=Open Typ=FFWCSX Listing=2016-05-04 12:00:00+00:00 Front=2016-05-04 12:00:00+00:00 PositionCurrency=USD Underlying=XBT QuoteCurrency=USD UnderlyingSymbol=XBT= Reference=BMEX ReferenceSymbol=.BXBT MaxOrderQty=10000000.0 MaxPrice=1000000.0 LotSize=100.0 TickSize=0.5 Multiplier=-100000000.0 SettlCurrency=XBt UnderlyingToSettleMultiplier=-100000000.0 IsInverse=True InitMargin=0.01 MaintMargin=0.0035 RiskLimit=20000000000.0 RiskStep=15000000000.0 Taxed=True Deleverage=True MakerFee=-0.0001 TakerFee=0.0005 FundingBaseSymbol=.XBTBON8H FundingQuoteSymbol=.USDBON8H FundingPremiumSymbol=.XBTUSDPI8H FundingTimestamp=2022-01-31 04:00:00+00:00 FundingInterval=2000-01-01 08:00:00+00:00 FundingRate=-0.004875 IndicativeFundingRate=-0.00225 OpeningTimestamp=2022-01-30 20:00:00+00:00 ClosingTimestamp=2022-01-30 21:00:00+00:00 SessionInterval=2000-01-01 01:00:00+00:00 PrevClosePrice=38134.04 PrevTotalVolume=147203953930.0 TotalVolume=147204169630.0 Volume=215700.0 Volume24h=3362300.0 PrevTotalTurnover=1953238412187379.0 TotalTurnover=1953238986458323.0 Turnover=574270944.0 Turnover24h=8898140695.0 HomeNotional24h=88.98140694999978 ForeignNotional24h=3362300.0 PrevPrice24h=37725.0 Vwap=37786.6114 HighPrice=38615.5 LowPrice=37225.0 LastPrice=37663.5 LastPriceProtected=37590.79 LastTickDirection=ZeroPlusTick LastChangePcnt=-0.0016 BidPrice=37663.0 MidPrice=37663.25 AskPrice=37663.5 ImpactBidPrice=37435.3772 ImpactMidPrice=37549.5 ImpactAskPrice=37663.5067 OpenInterest=66935400.0 OpenValue=178524074694.0 FairMethod=FundingRate FairBasisRate=-5.338125 FairBasis=-176.76 FairPrice=37493.83 MarkMethod=FairPrice MarkPrice=37493.83 IndicativeSettlePrice=37670.59 Timestamp=2022-01-30 20:18:45+00:00";
    env << "QUOTE Timestamp=2022-01-30 20:24:56.136000+00:00 Symbol=XBTUSD BidSize=200.0 BidPrice=37663.0 AskPrice=37663.5 AskSize=1078300.0";

    auto strategy = env.strategy();

    strategy->allocations()->addAllocation(37663.0, 100.0);
    strategy->allocations()->placeAllocations();
    auto order = env >> "ORDER Symbol=XBTUSD Side=Buy OrderQty=200.0 Price=37663.0 OrdType=Limit TimeInForce=GoodTillCancel ExDestination=XBME OrdStatus=New WorkingIndicator=True LeavesQty=200.0 MultiLegReportingType=SingleSecurity Text=Submitted via API. TransactTime=2022-01-30 20:10:17.266000+00:00 Timestamp=2022-01-30 20:10:17.266000+00:00";
    env << format("EXECUTION ExecId=16c53278-f1af-31ce-7117-e665a369a69f Symbol=XBTUSD Side=Sell LastQty=200.0 LastPx=37663.0 LastMkt=XBME LastLiquidityInd=AddedLiquidity OrderQty=200.0 Price=37663.0 Currency=USD SettlCurrency=XBt ExecType=Trade OrdType=Limit TimeInForce=GoodTillCancel ExDestination=XBME OrdStatus=Filled CumQty=200.0 AvgPx=37610.0 Commission=-0.0001 TradePublishIndicator=PublishTrade MultiLegReportingType=SingleSecurity Text=Submitted via API. TrdMatchId=1b32729e-b43a-9450-039e-15a60f43111c ExecCost=531774.0 ExecComm=-53.0 HomeNotional=-0.00531774 ForeignNotional=200.0 TransactTime=2022-01-30 19:44:36.002000+00:00 Timestamp=2022-01-30 19:44:36.002000+00:00", order);

    auto alloc = strategy->allocations()->get(37663.0);
    ASSERT_EQ(alloc->getOrder()->getAvgPx(), 37663.0);

    auto margin = strategy->getMD()->getMargin();
    auto cost = func::get_cost(37663.0, 200.0, 10);
    ASSERT_EQ(margin->getAmount(), 351951.0 - cost);
    ASSERT_EQ(margin->getMaintMargin(), 0.0);
    ASSERT_EQ(margin->getRealisedPnl(), 0.0);

    auto position = strategy->getMD()->getPositions().at("XBTUSD");

    ASSERT_EQ(position->getCurrentCost(), -2136362.0 + cost);

}
