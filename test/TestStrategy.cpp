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
        {"startingBalance", "1000000000"}
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
        {"startingBalance", "10000000000"},
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
        {"startingBalance", "1000000000"},
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




TEST(Strategy, balance_is_updated_during_test) {

    price_t startingBalance = 1000000000.0;

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
    auto expectedCurrentCost = 66733400.066733405;
    ASSERT_DOUBLE_EQ(position->getCurrentCost(), expectedCurrentCost);
    ASSERT_DOUBLE_EQ(position->getCurrentQty(), 100);
    // the bankruptcy price is invalid, as our leverage is > 1, so we will go bankrupt before price goes to zero
    ASSERT_DOUBLE_EQ(position->getAvgCostPrice(), 9.99);
    ASSERT_DOUBLE_EQ(position->getBankruptPrice(), 0.0);
    ASSERT_DOUBLE_EQ(position->getBreakEvenPrice(), 9.9899999999999988e-08);
    ASSERT_DOUBLE_EQ(position->getLiquidationPrice(), 0.23333333333332984);
    auto margin = env.strategy()->getMD()->getMargin();
    ASSERT_DOUBLE_EQ(margin->getWalletBalance(), 9.3333333333333339); // ~= startingBalance - currentCost
    ASSERT_DOUBLE_EQ(margin->getMaintMargin(), 0.035);
    ASSERT_DOUBLE_EQ(margin->getAvailableMargin(), 9.2983333333333338);
    ASSERT_DOUBLE_EQ(margin->getUnrealisedPnl(), 0.0);
}


TEST(Strategy, position_margin_is_updated_during_test) {

    price_t startingBalance = 3521000000;

    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "37663"},
        {"fairPrice", "37663"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
        {"startingBalance", std::to_string(startingBalance)}
    });
    env << "MARGIN Account=365570.0 Currency=XBt RiskLimit=1000000000000.0 Amount=351951.0 PrevRealisedPnl=-1959.0 GrossComm=-290.0 GrossMarkValue=1333800.0 RiskValue=1333800.0 MaintMargin=131305.0 RealisedPnl=88.0 UnrealisedPnl=-5588.0 WalletBalance=352039.0 MarginBalance=346451.0 MarginBalancePcnt=0.2597 MarginLeverage=3.8498950789577746 MarginUsedPcnt=0.379 ExcessMargin=215146.0 ExcessMarginPcnt=0.1613 AvailableMargin=215146.0 WithdrawableMargin=215146.0 Timestamp=2022-01-30 19:50:55.814000+00:00 GrossLastValue=1333800.0";
    env << "POSITION Account=365570.0 Symbol=XBTUSD Currency=XBt Underlying=XBT QuoteCurrency=USD Commission=0.0005 InitMarginReq=0.1 MaintMarginReq=0.0035 RiskLimit=20000000000.0 Leverage=10.0 DeleveragePercentile=0.4 PrevRealisedPnl=-1959.0 PrevClosePrice=37530.93 OpeningTimestamp=2022-01-30 20:00:00+00:00 OpeningQty=500.0 OpeningCost=-1328010.0 OpeningComm=-290.0 OpenOrderBuyQty=400.0 OpenOrderBuyCost=-1071848.0 ExecBuyQty=700.0 ExecBuyCost=1870472.0 ExecSellQty=400.0 ExecSellCost=1062120.0 ExecQty=300.0 ExecCost=-808352.0 ExecComm=-5805.0 CurrentTimestamp=2022-01-30 20:13:30.814000+00:00 CurrentQty=800.0 CurrentCost=-2136362.0 CurrentComm=-6095.0 RealisedCost=-248.0 UnrealisedCost=-2136114.0 GrossOpenCost=1071848.0 GrossExecCost=801631.0 IsOpen=True MarkPrice=37363.36 MarkValue=-2141136.0 RiskValue=3212984.0 HomeNotional=0.02141136 ForeignNotional=-800.0 PosCost=-2136114.0 PosCost2=-2136114.0 PosCross=2998.0 PosInit=213612.0 PosComm=1177.0 PosMargin=217787.0 PosMaint=8654.0 InitMargin=108311.0 MaintMargin=212765.0 RealisedGrossPnl=248.0 RealisedPnl=6343.0 UnrealisedGrossPnl=-5022.0 UnrealisedPnl=-5022.0 UnrealisedPnlPcnt=-0.0024 UnrealisedRoePcnt=-0.0235 AvgCostPrice=37451.2198 AvgEntryPrice=37451.2198 BreakEvenPrice=37340.5 MarginCallPrice=34112.0 LiquidationPrice=34112.0 BankruptPrice=34003.5 Timestamp=2022-01-30 20:13:30.814000+00:00 LastPrice=37363.36 LastValue=-2141136.0";
    env << "INSTRUMENT Symbol=XBTUSD RootSymbol=XBT State=Open Typ=FFWCSX Listing=2016-05-04 12:00:00+00:00 Front=2016-05-04 12:00:00+00:00 PositionCurrency=USD Underlying=XBT QuoteCurrency=USD UnderlyingSymbol=XBT= Reference=BMEX ReferenceSymbol=.BXBT MaxOrderQty=10000000.0 MaxPrice=1000000.0 LotSize=100.0 TickSize=0.5 Multiplier=-100000000.0 SettlCurrency=XBt UnderlyingToSettleMultiplier=-100000000.0 IsInverse=True InitMargin=0.01 MaintMargin=0.0035 RiskLimit=20000000000.0 RiskStep=15000000000.0 Taxed=True Deleverage=True MakerFee=-0.0001 TakerFee=0.0005 FundingBaseSymbol=.XBTBON8H FundingQuoteSymbol=.USDBON8H FundingPremiumSymbol=.XBTUSDPI8H FundingTimestamp=2022-01-31 04:00:00+00:00 FundingInterval=2000-01-01 08:00:00+00:00 FundingRate=-0.004875 IndicativeFundingRate=-0.00225 OpeningTimestamp=2022-01-30 20:00:00+00:00 ClosingTimestamp=2022-01-30 21:00:00+00:00 SessionInterval=2000-01-01 01:00:00+00:00 PrevClosePrice=38134.04 PrevTotalVolume=147203953930.0 TotalVolume=147204169630.0 Volume=215700.0 Volume24h=3362300.0 PrevTotalTurnover=1953238412187379.0 TotalTurnover=1953238986458323.0 Turnover=574270944.0 Turnover24h=8898140695.0 HomeNotional24h=88.98140694999978 ForeignNotional24h=3362300.0 PrevPrice24h=37725.0 Vwap=37786.6114 HighPrice=38615.5 LowPrice=37225.0 LastPrice=37663.5 LastPriceProtected=37590.79 LastTickDirection=ZeroPlusTick LastChangePcnt=-0.0016 BidPrice=37663.0 MidPrice=37663.25 AskPrice=37663.5 ImpactBidPrice=37435.3772 ImpactMidPrice=37549.5 ImpactAskPrice=37663.5067 OpenInterest=66935400.0 OpenValue=178524074694.0 FairMethod=FundingRate FairBasisRate=-5.338125 FairBasis=-176.76 FairPrice=37493.83 MarkMethod=FairPrice MarkPrice=37493.83 IndicativeSettlePrice=37670.59 Timestamp=2022-01-30 20:18:45+00:00";
    env << "QUOTE Timestamp=2022-01-30 20:24:56.136000+00:00 Symbol=XBTUSD BidSize=200.0 BidPrice=37663.0 AskPrice=37663.5 AskSize=1078300.0";

    auto strategy = env.strategy();
    auto margin = strategy->getMD()->getMargin();

    strategy->allocations()->addAllocation(37663.0, 100.0);
    strategy->allocations()->placeAllocations();
    auto order = env >> "ORDER_NEW Symbol=XBTUSD Side=Buy OrderQty=100.0 Price=37663.0 OrdType=Limit TimeInForce=GoodTillCancel ExDestination=XBME OrdStatus=New WorkingIndicator=True LeavesQty=200.0 MultiLegReportingType=SingleSecurity Text=Submitted via API. TransactTime=2022-01-30 20:10:17.266000+00:00 Timestamp=2022-01-30 20:10:17.266000+00:00";
    env << format("EXECUTION ExecId=16c53278-f1af-31ce-7117-e665a369a69f Symbol=XBTUSD Side=Sell LastQty=200.0 LastPx=37663.0 LastMkt=XBME LastLiquidityInd=AddedLiquidity OrderQty=200.0 Price=37663.0 Currency=USD SettlCurrency=XBt ExecType=Trade OrdType=Limit TimeInForce=GoodTillCancel ExDestination=XBME OrdStatus=Filled CumQty=200.0 AvgPx=37663.0 Commission=-0.0001 TradePublishIndicator=PublishTrade MultiLegReportingType=SingleSecurity TrdMatchId=1b32729e-b43a-9450-039e-15a60f43111c ExecCost=531774.0 ExecComm=-53.0 HomeNotional=-0.00531774 ForeignNotional=200.0 TransactTime=2022-01-30 19:44:36.002000+00:00 Timestamp=2022-01-30 19:44:36.002000+00:00", order);

    auto alloc = strategy->allocations()->get(37663.0);
    ASSERT_EQ(alloc->getOrder()->getAvgPx(), 37663.0);

    auto cost = func::get_cost(37663.0, 200.0, 10);
    ASSERT_EQ(margin->getAmount(), 351951.0 - cost);
    ASSERT_EQ(margin->getMaintMargin(), 0.0);
    ASSERT_EQ(margin->getRealisedPnl(), 0.0);

    auto position = strategy->getMD()->getPositions().at("XBTUSD");

    ASSERT_EQ(position->getCurrentCost(), -2136362.0 + cost);

}


TEST(Strategy, test_new_order_and_fill) {
    TestEnv env({DEFAULT_ARGS,
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"referencePrice", "37663"},
        {"fairPrice", "37663"},
        {"shortTermWindow", "100"},
        {"longTermWindow", "1000"},
    });

    env <<  "INSTRUMENT " // Instrument data
         "Symbol=XBTUSD "
         "RootSymbol=XBT "
         "State=Open "
         "Typ=FFWCSX "
         "Listing=2016-05-04 12:00:00+00:00 "
         "Front=2016-05-04 12:00:00+00:00 "
         "Expiry= "
         "Settle= "
         "RelistInterval= "
         "InverseLeg= "
         "SellLeg= "
         "BuyLeg= "
         "OptionStrikePcnt= "
         "OptionStrikeRound= "
         "OptionStrikePrice= "
         "OptionMultiplier= "
         "PositionCurrency=USD "
         "Underlying=XBT "
         "QuoteCurrency=USD "
         "UnderlyingSymbol=XBT= "
         "Reference=BMEX "
         "ReferenceSymbol=.BXBT "
         "CalcInterval= "
         "PublishInterval= "
         "PublishTime= "
         "MaxOrderQty=10000000.0 "
         "MaxPrice=1000000.0 "
         "LotSize=100.0 "
         "TickSize=0.5 "
         "Multiplier=-100000000.0 "
         "SettlCurrency=XBt "
         "UnderlyingToPositionMultiplier= "
         "UnderlyingToSettleMultiplier=-100000000.0 "
         "QuoteToSettleMultiplier= "
         "IsQuanto= "
         "IsInverse=True "
         "InitMargin=0.01 "
         "MaintMargin=0.0035 "
         "RiskLimit=20000000000.0 "
         "RiskStep=15000000000.0 "
         "Limit= "
         "Capped= "
         "Taxed=True "
         "Deleverage=True "
         "MakerFee=-0.0001 "
         "TakerFee=0.0005 "
         "SettlementFee= "
         "InsuranceFee= "
         "FundingBaseSymbol=.XBTBON8H "
         "FundingQuoteSymbol=.USDBON8H "
         "FundingPremiumSymbol=.XBTUSDPI8H "
         "FundingTimestamp=2022-02-20 04:00:00+00:00 "
         "FundingInterval=2000-01-01 08:00:00+00:00 "
         "FundingRate=-0.002359 "
         "IndicativeFundingRate=-0.001022 "
         "RebalanceTimestamp= "
         "RebalanceInterval= "
         "OpeningTimestamp=2022-02-19 22:00:00+00:00 "
         "ClosingTimestamp=2022-02-19 23:00:00+00:00 "
         "SessionInterval=2000-01-01 01:00:00+00:00 "
         "PrevClosePrice=39858.29 "
         "LimitDownPrice= "
         "LimitUpPrice= "
         "BankruptLimitDownPrice= "
         "BankruptLimitUpPrice= "
         "PrevTotalVolume=147459997130.0 "
         "TotalVolume=147460043130.0 "
         "Volume=46000.0 "
         "Volume24h=815600.0 "
         "PrevTotalTurnover=1953843821710034.0 "
         "TotalTurnover=1953843936881753.0 "
         "Turnover=115171719.0 "
         "Turnover24h=2039776620.0 "
         "HomeNotional24h=20.39776620000001 "
         "ForeignNotional24h=815600.0 "
         "PrevPrice24h=39709.0 "
         "Vwap=39984.8058 "
         "HighPrice=40222.0 "
         "LowPrice=39672.0 "
         "LastPrice=39914.0 "
         "LastPriceProtected=39913.28 "
         "LastTickDirection=ZeroMinusTick "
         "LastChangePcnt=0.0052 "
         "BidPrice=39914.0 "
         "MidPrice=39916.25 "
         "AskPrice=39918.5 "
         "ImpactBidPrice=39811.9285 "
         "ImpactMidPrice=39910.25 "
         "ImpactAskPrice=40008.3217 "
         "HasLiquidity= "
         "OpenInterest=66125500.0 "
         "OpenValue=165963102410.0 "
         "FairMethod=FundingRate "
         "FairBasisRate=-2.5831049999999998 "
         "FairBasis=-66.3 "
         "FairPrice=39843.55 "
         "MarkMethod=FairPrice "
         "MarkPrice=39843.55 "
         "IndicativeTaxRate= "
         "IndicativeSettlePrice=39909.85 "
         "OptionUnderlyingPrice= "
         "SettledPrice= "
         "Timestamp=2022-02-19 22:22:46.337000+00:00 ";
    env <<  "MARGIN " // Initial margin
         "Amount=150574.0 "
         "PrevRealisedPnl=1770.0 "
         "GrossComm=-1322.0 "
         "MaintMargin=24921.0 "
         "RealisedPnl=-13609.0 "
         "UnrealisedPnl=-447.0 "
         "WalletBalance=136965.0 "
         "MarginBalance=136518.0 "
         "MarginBalancePcnt=0.5439 "
         "MarginLeverage=1.8384535372624855 "
         "MarginUsedPcnt=0.3736 "
         "ExcessMargin=85517.0 "
         "ExcessMarginPcnt=0.3407 "
         "AvailableMargin=85517.0 ";
    env << "POSITION " // Initial position
         "Symbol=XBTUSD "
         "Currency=XBt "
         "Underlying=XBT "
         "QuoteCurrency=USD "
         "Commission=0.0005 "
         "InitMarginReq=0.1 "
         "MaintMarginReq=0.0035 "
         "RiskLimit=20000000000.0 "
         "Leverage=10.0 "
         "PrevClosePrice=39873.54 "
         "OpeningTimestamp=2022-02-19 22:00:00+00:00 "
         "OpeningQty=500.0 "
         "OpeningCost=-1235715.0 "
         "OpeningComm=-1422.0 "
         "ExecSellQty=600.0 "
         "ExecSellCost=1501182.0 "
         "ExecQty=-400.0 "
         "ExecCost=1000111.0 "
         "ExecComm=100.0 "
         "CurrentQty=100.0 "
         "CurrentCost=-235604.0 "
         "CurrentComm=-1322.0 "
         "RealisedCost=14931.0 "
         "UnrealisedCost=-250535.0 "
         "GrossExecCost= "
         "IsOpen=True "
         "MarkPrice=39843.55 "
         "MarkValue=-250982.0 "
         "RiskValue=509066.0 "
         "HomeNotional=0.00250982 "
         "ForeignNotional=-100.0 "
         "PosCost=-250535.0 "
         "PosCross=176.0 "
         "PosComm=138.0 "
         "PosMargin=25368.0 "
         "PosMaint=1015.0 "
         "MaintMargin=24921.0 "
         "RealisedGrossPnl=-14931.0 "
         "RealisedPnl=-13609.0 "
         "UnrealisedGrossPnl=-447.0 "
         "UnrealisedPnl=-447.0 "
         "UnrealisedPnlPcnt=-0.0018 "
         "UnrealisedRoePcnt=-0.0178 "
         "AvgCostPrice=39914.6 "
         "AvgEntryPrice=39914.6 "
         "BreakEvenPrice=39896.5 "
         "MarginCallPrice=36378.5 "
         "LiquidationPrice=36378.5 "
         "BankruptPrice=36263.0 "
         "Timestamp=2022-02-19 22:22:47.523000+00:00 "
         "LastPrice=39843.55 "
         "LastValue=-250982.0 ";
    env <<  "QUOTE " // quote
         "Timestamp=2022-02-19 22:21:10.876000+00:00 "
         "Symbol=XBTUSD "
         "BidSize=100.0 "
         "BidPrice=39965.5 "
         "AskPrice=39993.5 "
         "AskSize=700.0 ";
    env >>  "ORDER " // quote
         "OrderId=4a9bd80b-abab-40df-b5d4-f0ae866d4019 "
         "ClOrdId=MCST_9be829a "
         "ClOrdLinkId= "
         "Account=365570.0 "
         "Symbol=XBTUSD "
         "Side=Buy "
         "SimpleOrderQty= "
         "OrderQty=100.0 "
         "Price=39918.5 "
         "TimeInForce=Day "
         "ExecInst= "
         "ContingencyType= "
         "ExDestination=XBME "
         "OrdStatus=Filled "
         "Triggered= "
         "WorkingIndicator= "
         "OrdRejReason= "
         "SimpleLeavesQty= "
         "LeavesQty= "
         "SimpleCumQty= "
         "CumQty=100.0 "
         "AvgPx=39918.5 "
         "MultiLegReportingType=SingleSecurity "
         "Text=Submitted via API. "
         "TransactTime=2022-02-19 22:22:48.437000+00:00 "
         "Timestamp=2022-02-19 22:22:48.437000+00:00 ";
    env >>  "MARGIN " // margin post fill
         "Amount=150574.0 "
         "PrevRealisedPnl=1770.0 "
         "GrossComm=-1197.0 "
         "MaintMargin=49638.0 "
         "RealisedPnl=-13734.0 "
         "UnrealisedPnl=-919.0 "
         "WalletBalance=136840.0 "
         "MarginBalance=135921.0 "
         "MarginBalancePcnt=0.2708 "
         "MarginLeverage=3.693056996343464 "
         "MarginUsedPcnt=0.5571 "
         "ExcessMargin=60203.0 "
         "ExcessMarginPcnt=0.1199 "
         "AvailableMargin=60203.0 ";
    env >>  "POSITION " // position post fill
         "Symbol=XBTUSD "
         "Currency=XBt "
         "Underlying=XBT "
         "QuoteCurrency=USD "
         "Commission=0.0005 "
         "InitMarginReq=0.1 "
         "MaintMarginReq=0.0035 "
         "RiskLimit=20000000000.0 "
         "Leverage=10.0 "
         "PrevClosePrice=39873.54 "
         "OpeningTimestamp=2022-02-19 22:00:00+00:00 "
         "OpeningQty=500.0 "
         "OpeningCost=-1235715.0 "
         "OpeningComm=-1422.0 "
         "ExecSellQty=600.0 "
         "ExecSellCost=1501182.0 "
         "ExecQty=-300.0 "
         "ExecCost=749601.0 "
         "ExecComm=225.0 "
         "CurrentQty=200.0 "
         "CurrentCost=-486114.0 "
         "CurrentComm=-1197.0 "
         "RealisedCost=14931.0 "
         "UnrealisedCost=-501045.0 "
         "GrossExecCost= "
         "IsOpen=True "
         "MarkPrice=39843.55 "
         "MarkValue=-501964.0 "
         "RiskValue=760048.0 "
         "HomeNotional=0.00501964 "
         "ForeignNotional=-200.0 "
         "PosCost=-501045.0 "
         "PosCross=176.0 "
         "PosComm=276.0 "
         "PosMargin=50557.0 "
         "PosMaint=2030.0 "
         "MaintMargin=49638.0 "
         "RealisedGrossPnl=-14931.0 "
         "RealisedPnl=-13734.0 "
         "UnrealisedGrossPnl=-919.0 "
         "UnrealisedPnl=-919.0 "
         "UnrealisedPnlPcnt=-0.0018 "
         "UnrealisedRoePcnt=-0.0183 "
         "AvgCostPrice=39916.654 "
         "AvgEntryPrice=39916.654 "
         "BreakEvenPrice=39917.5 "
         "MarginCallPrice=36392.0 "
         "LiquidationPrice=36392.0 "
         "BankruptPrice=36276.5 "
         "Timestamp=2022-02-19 22:22:48.437000+00:00 "
         "LastPrice=39843.55 "
         "LastValue=-501964.0 ";

}
