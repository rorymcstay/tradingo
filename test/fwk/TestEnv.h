//
// Created by Rory McStay on 22/06/2021.
//

#ifndef TRADINGO_TESTENV_H
#define TRADINGO_TESTENV_H
#include <functional>
#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <variant>

#define _TURN_OFF_PLATFORM_STRING
#include "TestMarketData.h"
#include "TestOrdersApi.h"
#include "TestPositionApi.h"

#include "Strategy.h"
#include "OrderInterface.h"
#include "Utils.h"
#include "Context.h"
#include "Series.h"

#include "model/Execution.h"
#include "model/Position.h"
#include "model/Instrument.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define FILENAME(x) __FILENAME__#x
#define LN  " | " __FILE__ ":" TO_STRING(__LINE__)

using namespace io::swagger::client;



#define DEFAULT_ARGS \
        {"symbol", "XBTUSD"},\
        {"clOrdPrefix", "MCST"},\
        {"factoryMethod", "RegisterBreakOutStrategy"},\
        {"displaySize", "200"},\
        {"referencePrice", "35000"},\
        {"fairPrice", "35000"},\
        {"shortTermWindow", "1000"},\
        {"longTermWindow", "8000"},\
        {"moving_average_crossover-interval", "1000"},\
        {"signal-callback", "1000"},\
        {"logLevel", "debug"},\
        {"moving_average_crossover-callback", "false"}, \
        {"realtime", "false"},                         \
        {"override-signal-callback", "true"},         \
        {"startingBalance", "0.001"}, \
        {"initialLeverage", "15.0"}, \
        {"libraryLocation", LIBRARY_LOCATION"/libtest_trading_strategies.so" }, \
        {"storage", "./"},   \
        {"tickStorage", std::getenv("TICK_STORAGE") ? std::getenv("TICK_STORAGE") : "./"}

#define INSERT_DEFAULT_ARGS(config_) \
    std::initializer_list<std::pair<std::string, std::string>> defaults = { DEFAULT_ARGS }; \
    for (auto pair : defaults) \
        (config_)->set(pair.first, pair.second);

#define DEFAULT_START \
    env << "INSTRUMENT Symbol=XBTUSD RootSymbol=XBT State=Open Typ=FFWCSX Listing=2016-05-04 12:00:00+00:00 Front=2016-05-04 12:00:00+00:00 PositionCurrency=USD Underlying=XBT QuoteCurrency=USD UnderlyingSymbol=XBT= Reference=BMEX ReferenceSymbol=.BXBT MaxOrderQty=10000000.0 MaxPrice=1000000.0 LotSize=100.0 TickSize=0.5 Multiplier=-100000000.0 SettlCurrency=XBt UnderlyingToSettleMultiplier=-100000000.0 IsInverse=True InitMargin=0.01 MaintMargin=0.0035 RiskLimit=20000000000.0 RiskStep=15000000000.0 Taxed=True Deleverage=True MakerFee=-0.0001 TakerFee=0.0005 FundingBaseSymbol=.XBTBON8H FundingQuoteSymbol=.USDBON8H FundingPremiumSymbol=.XBTUSDPI8H FundingTimestamp=2022-02-02 04:00:00+00:00 FundingInterval=2000-01-01 08:00:00+00:00 FundingRate=-0.00229 IndicativeFundingRate=-0.000828 OpeningTimestamp=2022-02-01 21:00:00+00:00 ClosingTimestamp=2022-02-01 22:00:00+00:00 SessionInterval=2000-01-01 01:00:00+00:00 PrevClosePrice=38480.52 PrevTotalVolume=147217012830.0 TotalVolume=147217035030.0 Volume=22200.0 Volume24h=8262500.0 PrevTotalTurnover=1953272677620034.0 TotalTurnover=1953272734952874.0 Turnover=57332840.0 Turnover24h=21451499082.0 HomeNotional24h=214.5149908200005 ForeignNotional24h=8262500.0 PrevPrice24h=38465.5 Vwap=38517.2403 HighPrice=39212.0 LowPrice=37973.0 LastPrice=38780.5 LastPriceProtected=38780.5 LastTickDirection=PlusTick LastChangePcnt=0.0082 BidPrice=38798.5 MidPrice=38798.75 AskPrice=38799.0 ImpactBidPrice=38684.5699 ImpactMidPrice=38751.5 ImpactAskPrice=38818.6702 HasLiquidity=True OpenInterest=70754700.0 OpenValue=182755852365.0 FairMethod=FundingRate FairBasisRate=-2.5075499999999997 FairBasis=-71.06 FairPrice=38715.36 MarkMethod=FairPrice MarkPrice=38715.36 IndicativeSettlePrice=38786.42 Timestamp=2022-02-01 21:36:55+00:00"; \
    env << "MARGIN Account=365570.0 Currency=XBt RiskLimit=1000000000000.0 Amount=399938.0 PrevRealisedPnl=-167.0 GrossComm=-4502.0 GrossOpenCost=1298870.0 GrossMarkValue=774885.0 RiskValue=2073755.0 InitMargin=131251.0 MaintMargin=82677.0 RealisedPnl=-22875.0 UnrealisedPnl=4326.0 WalletBalance=377063.0 MarginBalance=381389.0 MarginBalancePcnt=0.4922 MarginLeverage=2.031744491844285 MarginUsedPcnt=0.5609 ExcessMargin=167461.0 ExcessMarginPcnt=0.2161 AvailableMargin=167461.0 WithdrawableMargin=167461.0 Timestamp=2022-02-01 21:36:56.314000+00:00 GrossLastValue=774885.0"; \
    env << "POSITION Account=365570.0 Symbol=XBTUSD Currency=XBt Underlying=XBT QuoteCurrency=USD Commission=0.0005 InitMarginReq=0.1 MaintMarginReq=0.0035 RiskLimit=20000000000.0 Leverage=10.0 DeleveragePercentile=0.4 RebalancedPnl=-11532.0 PrevRealisedPnl=-167.0 PrevClosePrice=38468.74 OpeningTimestamp=2022-02-01 21:00:00+00:00 OpeningQty=300.0 OpeningCost=-751834.0 OpeningComm=-4502.0 OpenOrderBuyQty=500.0 OpenOrderBuyCost=-1298870.0 CurrentTimestamp=2022-02-01 21:36:56.314000+00:00 CurrentQty=300.0 CurrentCost=-751834.0 CurrentComm=-4502.0 RealisedCost=27377.0 UnrealisedCost=-779211.0 GrossOpenCost=1298870.0 IsOpen=True MarkPrice=38715.36 MarkValue=-774885.0 RiskValue=2073755.0 HomeNotional=0.00774885 ForeignNotional=-300.0 PosCost=-779211.0 PosCost2=-779211.0 PosInit=77922.0 PosComm=429.0 PosMargin=78351.0 PosMaint=3157.0 InitMargin=131251.0 MaintMargin=82677.0 RealisedGrossPnl=-27377.0 RealisedPnl=-22875.0 UnrealisedGrossPnl=4326.0 UnrealisedPnl=4326.0 UnrealisedPnlPcnt=0.0056 UnrealisedRoePcnt=0.0555 AvgCostPrice=38500.5 AvgEntryPrice=38500.5 BreakEvenPrice=40279.5 MarginCallPrice=35112.5 LiquidationPrice=35112.5 BankruptPrice=35000.5 Timestamp=2022-02-01 21:36:56.314000+00:00 LastPrice=38715.36 LastValue=-774885.0"; \
    env << "QUOTE Timestamp=2022-02-01 21:36:08.046000+00:00 Symbol=XBTUSD BidSize=100.0 BidPrice=38745.0 AskPrice=38745.5 AskSize=100.0";


/// used to dedupe series
struct trade_equal {
    bool operator()(const std::shared_ptr<model::Trade>& a,
            const std::shared_ptr<model::Trade>& b) const {
        // Remove identical trd match ids
        return (a->getTrdMatchID() == b->getTrdMatchID());
    }
};

struct trade_hasher {
    std::size_t operator()(const std::shared_ptr<model::Trade>& v) const {
        return std::hash<std::string>{}(v->getTrdMatchID());
    }
};

struct instrument_equal {
    bool operator()(const std::shared_ptr<model::Instrument>& a,
            const std::shared_ptr<model::Instrument>& b) const {
        // Remove identical trd match ids
        return (a->getTimestamp() == b->getTimestamp());
    }
};

struct instrument_hasher {
    std::size_t operator()(const std::shared_ptr<model::Instrument>& v) const {
        return v->getTimestamp().to_interval();
    }
};

struct quotes_equal {
    bool operator()(const std::shared_ptr<model::Quote>& a,
            const std::shared_ptr<model::Quote>& b) const {
        // Remove identical trd match ids
        return (a->getTimestamp() == b->getTimestamp());
    }
};
struct quotes_hasher {
    std::size_t operator()(const std::shared_ptr<model::Quote>& v) const {
        return v->getTimestamp().to_interval();
    }
};




class TestEnv {
    using OrderApi = TestOrdersApi;
    using PositionApi = TestPositionApi;
    using TStrategy = Strategy<OrderApi, PositionApi>;
    using TContext = std::shared_ptr<Context<TestMarketData, OrderApi, TestPositionApi>>;


    std::shared_ptr<Config> _config;
    std::shared_ptr<Context<TestMarketData, OrderApi, PositionApi>> _context;
    long _events;

    TestOrdersApi::Writer _orderWriter;
    TestOrdersApi::Writer _executionWriter;
    TestOrdersApi::Writer _positionWriter;
    TestOrdersApi::Writer _marginWriter;

    void liquidatePositions(const std::string&);

    void updatePositionFromInstrument(const std::shared_ptr<model::Instrument>& instrument);

    void writePosition(const std::shared_ptr<model::Position>& position_);
    void writeMargin(const std::shared_ptr<model::Margin>& margin_);
    void updateMargin(const utility::datetime& time_);

public:
    using InstrumentSeries = Series<model::Instrument, instrument_equal, instrument_hasher>;
    using QuoteSeries = Series<model::Quote, quotes_equal, quotes_hasher>;
    using TradeSeries = Series<model::Trade, trade_equal, trade_hasher>;
    /// strategy reference
    const std::shared_ptr<TStrategy>& strategy() const { return _context->strategy(); }
    /// context reference
    const TContext& context() const { return _context; }
    /// unittest constructor
    TestEnv(std::initializer_list<std::pair<std::string, std::string>>);
    /// constructor from already initialised config
    TestEnv(const std::shared_ptr<Config>& config_);
    /// test setup called on construction
    void init();
    /// playback over a file
    void playback(const TradeSeries& trades,
                   const QuoteSeries& quotes,
                   const InstrumentSeries& instruments);
 
    // unittest event operators
    void operator << (const std::string& value_);
    std::shared_ptr<model::Order> operator >> (const std::string& value_);

    // low level object event operators
    void operator << (const std::shared_ptr<model::Position>&);
    void operator << (const std::shared_ptr<model::Margin>&);
    void operator << (const std::shared_ptr<model::Instrument>&);
    void operator << (const std::shared_ptr<model::Quote>&);
    void operator << (const std::shared_ptr<model::Execution>&);

    // trade simulator operator
    std::shared_ptr<model::Execution> operator << (const std::shared_ptr<model::Trade>&);

    /// test assertion on position
    void operator >> (const std::shared_ptr<model::Position>&);
    // test assertion on margin
    void operator >> (const std::shared_ptr<model::Margin>&);
    /// simulate sending an order
    std::shared_ptr<model::Order> operator >> (const std::shared_ptr<model::Order>&);
};


std::string format(const std::string& test_message_, const std::shared_ptr<model::Order>& order_);


#endif //TRADINGO_TESTENV_H
