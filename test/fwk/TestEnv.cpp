#include "aixlog.hpp"
#include <gtest/gtest.h>
#include <iomanip>
#include <chrono>
#include <cpprest/json.h>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#define _TURN_OFF_PLATFORM_STRING
#include "TestEnv.h"
#include "Utils.h"

#include <model/Margin.h>
#include "model/Execution.h"
#include "model/Trade.h"

#include "Context.h"
#include "BatchWriter.h"
#include "Series.h"
#include "Functional.h"


/// get test meta file
web::json::value get_validation_fields() {
    std::string validation_fields_file = TESTDATA_LOCATION "/validation_fields.json";
    std::ifstream t(validation_fields_file);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string val = buffer.str();
    return web::json::value::parse(val);
}


TestEnv::TestEnv(const std::shared_ptr<Config> &config_)
:   _config(config_)
,   _position(std::make_shared<model::Position>())
,   _margin(std::make_shared<model::Margin>())
,   _positionWriter("replay_positions",
        _config->get("symbol"),
        _config->get("storage"), 1,
        [](const std::shared_ptr<model::ModelBase>& order_) {
                return order_->toJson().serialize();
    }, /*rotate=*/false)
,   _batchWriter("replay_orders",
        _config->get("symbol"),
        _config->get("storage"), 1,
        [](const std::shared_ptr<model::ModelBase>& order_) {
                return order_->toJson().serialize();
    }, /*rotate=*/false)
{
    init();
}


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _config(std::make_shared<Config>(config_))
,   _position(std::make_shared<model::Position>())
,   _margin(std::make_shared<model::Margin>())
,   _positionWriter("replay_positions",
        _config->get("symbol"),
        _config->get("storage"), 1,
        [](const std::shared_ptr<model::ModelBase>& order_) {
                return order_->toJson().serialize();
    }, /*rotate=*/false)
,   _batchWriter("replay_orders",
        _config->get("symbol"),
        _config->get("storage"), 1,
        [](const std::shared_ptr<model::ModelBase>& order_) {
                return order_->toJson().serialize();
    }, /*rotate=*/false)
{
    init();
}


void TestEnv::init() {

    _config->set("baseUrl", "https://localhost:8888/api/v1");
    _config->set("apiKey", "dummy");
    _config->set("apiSecret", "dummy");
    _config->set("connectionString", "https://localhost:8888/realtime");
    _config->set("clOrdPrefix", "MCST");
    _config->set("httpEnabled", "False");
    _config->set("tickSize", "0.5");
    _config->set("lotSize", "100");
    _config->set("callback-signals", "true");
    _config->set("cloidSeed", "0");


    _context = std::make_shared<Context<TestMarketData, OrderApi, TestPositionApi>>(_config);

    // make instrument for test
    auto tickSize = std::atof(_config->get("tickSize", "0.5").c_str());
    auto referencePrice = std::atof(_config->get("referencePrice").c_str());
    auto lotSize = std::atof(_config->get("lotSize", "100").c_str());
    auto fairPrice = std::atof(_config->get("fairPrice").c_str());
    auto instSvc = std::shared_ptr<InstrumentService>(nullptr);
    auto instrument = std::make_shared<model::Instrument>();
    auto maintMargin = std::atof(_config->get("maintMargin", "0.035").c_str());
    instrument->setFairPrice(fairPrice);
    instrument->setSymbol(_config->get("symbol"));
    instrument->setPrevPrice24h(referencePrice);
    instrument->setTickSize(tickSize);
    instrument->setLotSize(lotSize);
    instrument->setMaintMargin(maintMargin);
    instrument->setMarkPrice(fairPrice);
    instrument->setTimestamp(utility::datetime::utc_now());
    *_context->marketData() << instrument;

    // setup initial positoin
    _position->setSymbol(_config->get("symbol"));
    _position->setCurrentQty(0.0);
    _position->setCurrentCost(0.0);
    _position->setUnrealisedPnl(0.0);
    _position->setTimestamp(utility::datetime::utc_now());
    _position->setLeverage(10.0);
    *_context->marketData() << _position;
    _context->orderApi()->setMarketData(_context->marketData());
    _context->positionApi()->setMarketData(_context->marketData());

    // set up initial margin
    _context->orderApi()->setMarginCalculator(
        std::make_shared<MarginCalculator>(_config, _context->marketData())
    );
    _margin->setWalletBalance(std::atof(_config->get("startingBalance").c_str()));
    _margin->setMaintMargin(0.0);
    _margin->setAvailableMargin(_margin->getWalletBalance());
    _margin->setUnrealisedPnl(0.0);
    _margin->setCurrency("XBt");
    _margin->setTimestamp(utility::datetime::utc_now());
    *_context->marketData() << _margin;

    _context->orderApi()->init(_config);
    // initialise strategy as the very last thing we do
    _context->init();
    _context->initStrategy();

}


void TestEnv::operator<<(const std::string &value_) {
    try {
        if (getEventTypeFromString(value_) == "EXECUTION") {
            auto params = Params(value_);
            auto exec = std::make_shared<model::Execution>();
            auto json = params.asJson();
            exec->fromJson(json);
            operator<<(exec);
        } else {
            *_context->marketData() << value_;
        }
    } catch (std::exception& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event <<\n\n      " << value_;
    }
}


std::shared_ptr<model::Order> TestEnv::operator>>(const std::string &value_) {
    try {
        *_context->orderApi() >> value_;
        auto eventType = getEventTypeFromString(value_);
        return _context->orderApi()->getEvent(eventType);
    } catch (std::runtime_error& ex) {
        std::stringstream error_msg;
        error_msg << "TEST Exception: " << ex.what() << " during event >>\n\n      " << value_;
        //FAIL() << error_msg;
        throw ex;
    }
}


std::string format(const std::string& test_message_,
                   const std::shared_ptr<model::Order>& order_) {
    std::stringstream ss;
    ss << test_message_ << " OrderID=" << order_->getOrderID()
        << " ClOrdID=" << order_->getClOrdID();
       if (order_->origClOrdIDIsSet())
           ss << " OrigClOrdID=" << order_->getOrigClOrdID();
    return ss.str();
}

/// if trade may not occur, return nullptr, else return exec report and modify order.
std::shared_ptr<model::Execution> canTrade(const std::shared_ptr<model::Order>& order_,
        const std::shared_ptr<model::Trade>& trade_) {



}

struct PlaybackStats {
    utility::datetime start_time;
    utility::datetime end_time;
    utility::datetime current_time;

    long quotes_size;
    long trades_size;
    long instruments_size;
    std::shared_ptr<model::Position> position;

    long quotes_processed = 0;
    long instruments_processed = 0;
    long trades_processed = 0;
    std::chrono::system_clock::time_point last_log;
    std::chrono::system_clock::duration interval = std::chrono::seconds(10);

};

#define LOG_STATS(stats_) \
        double quotes_progress = 100*(stats_.quotes_processed/(double)stats_.quotes_size); \
        double instruments_progress = 100*(stats.instruments_processed/(double)stats_.instruments_size); \
        double trades_progress = 100*(stats.trades_processed/(double)stats_.trades_size); \
        LOGWARN("STATS: " \
            << LOG_NVP("QuoteProgress",  quotes_progress) \
            << LOG_NVP("TradeProgress",  trades_progress) \
            << LOG_NVP("InstrumentProgress", instruments_progress) \
            << LOG_NVP("StartTime", stats_.start_time.to_string(utility::datetime::date_format::ISO_8601)) \
            << LOG_NVP("CurrentTime", stats_.current_time.to_string(utility::datetime::date_format::ISO_8601)) \
            << LOG_NVP("EndTime", stats_.end_time.to_string(utility::datetime::date_format::ISO_8601)) \
            <<LOG_NVP("Position", stats_.position->toJson().serialize()) \
            );


void TestEnv::playback(const Series<model::Trade>& trades,
                       const Series<model::Quote>& quotes,
                       const Series<model::Instrument>& instruments) {

    auto quote = quotes.begin();
    auto trade = trades.begin();
    auto instrument = instruments.begin();

    auto stats = PlaybackStats {
        quotes.begin()->getTimestamp(),
        quotes.end()->getTimestamp(),
        quotes.begin()->getTimestamp(),
        quotes.size(),
        trades.size(),
        instruments.size(),
        _position
    };

    std::vector<std::shared_ptr<model::ModelBase>> outBuffer;

    auto symbol = _config->get("symbol");
    
    utility::datetime::interval_type max_interval = std::numeric_limits<utility::datetime::interval_type>::max();

    while (quote != quotes.end() or trade != trades.end() or instrument != instruments.end()) {
    
        utility::datetime::interval_type current_time = std::min({
                (quote != quotes.end()) ? quote->getTimestamp().to_interval() : max_interval,
                (trade != trades.end()) ? trade->getTimestamp().to_interval() : max_interval,
                (instrument != instruments.end()) ? instrument->getTimestamp().to_interval() : max_interval
        });
        // TRADE
        if (trade != trades.end() and trade->getTimestamp().to_interval() == current_time) {
            auto exec = operator<<(*trade);
            if (exec) {
                operator<<(exec);
            }
            ++trade;
            stats.trades_processed += 1;
            stats.current_time = trade->getTimestamp();
        // QUOTE
        } else if (quote != quotes.end() and quote->getTimestamp().to_interval() == current_time) {
            (*_context->orderApi()->getMarginCalculator())(*quote);
            operator<<(*quote);
            ++quote;
            stats.quotes_processed += 1;
            stats.current_time = quote->getTimestamp();
        // INSTRUMENT
        } else if (instrument != instruments.end() and instrument->getTimestamp().to_interval() == current_time) {
            auto position = _context->marketData()->getPositions().at(instrument->getSymbol());
            if ((position->getCurrentQty() > 0.0 && instrument->getMarkPrice() <= position->getLiquidationPrice())
                    or(position->getCurrentQty() < 0.0 && instrument->getMarkPrice() >= position->getMarkPrice()))
            {
                liquidatePositions(position->getSymbol());
            }
            operator<<(*instrument);
            ++instrument;
            stats.instruments_processed += 1;
            stats.current_time = instrument->getTimestamp();
        } else {
            std::stringstream ss;
            if (not (trade != trades.end())) { ss << "Trades "; }
            else if (not(quote != quotes.end())) { ss << "Quotes ";}
            else if (not(instrument != instruments.end())) { ss << "Instruments ";} 
            else { ss << "UNKNOWN "; }
            ss << "Has completed.";
            LOGWARN(ss.str());
        }
        if (std::chrono::system_clock::now() - stats.last_log > stats.interval) {
            LOG_STATS(stats);
            stats.last_log = std::chrono::system_clock::now();
        }
    }
    _batchWriter.write_batch();
    _positionWriter.write_batch();
}


void TestEnv::liquidatePositions(const std::string& symbol) {
    std::shared_ptr<MarginCalculator> mc = _context->orderApi()->getMarginCalculator();
   
    auto md = _context->strategy()->getMD();
    
    auto position = md->getPositions().at(symbol);

    auto qty = position->getCurrentQty();
    auto margin = md->getMargin();
    auto liqPrice = position->getLiquidationPrice();
    auto markPrice = mc->getMarkPrice();

    position->setPosState("Liquidation");
    auto execution = std::make_shared<model::Execution>();
    execution->setLastPx(position->getLiquidationPrice());
    execution->setLastQty(std::abs(position->getCurrentQty()));
    execution->setSide(position->getCurrentQty() >= 0.0 ? "Sell" : "Buy");
    execution->setAvgPx(position->getLiquidationPrice());
    execution->setCumQty(position->getCurrentQty());
    execution->setText("Liquidation");
    execution->setLastLiquidityInd("RemovedLiquidity");
    execution->setOrderQty(std::abs(position->getCurrentQty()));

    LOGINFO("Auto liquidation triggered: "
                    << LOG_VAR(liqPrice) << LOG_VAR(markPrice) << LOG_VAR(qty)
                    << LOG_NVP("balance", margin->getWalletBalance()));
    operator<<(execution);

}


void TestEnv::operator << (const std::shared_ptr<model::Position>& position_) {
    *_context->marketData() << position_;
}


void TestEnv::operator << (const std::shared_ptr<model::Execution>& exec_) {
    *_context->marketData() << exec_;
    auto& position = _context->marketData()->getPositions().at(exec_->getSymbol());
    auto& order = _context->orderApi()->orders().at(exec_->getClOrdID());
    auto& instrument = _context->marketData()->getInstruments().at(exec_->getSymbol());
    auto& margin = _context->marketData()->getMargin();

    // TODO
    // account: Your unique account ID.
    /*
    symbol: The contract for this position.
    currency: The margin currency for this position.
    underlying: Meta data of the symbol.
    quoteCurrency: Meta data of the symbol, All prices are in the quoteCurrency
    commission: The maximum of the maker, taker, and settlement fee.
    initMarginReq: The initial margin requirement. This will be at least the symbol's default initial maintenance margin, but can be higher if you choose lower leverage.
    maintMarginReq: The maintenance margin requirement. This will be at least the symbol's default maintenance maintenance margin, but can be higher if you choose a higher risk limit.
    riskLimit: This is a function of your maintMarginReq.
    leverage: 1 / initMarginReq.
    crossMargin: True/false depending on whether you set cross margin on this position.
    deleveragePercentile: Indicates where your position is in the ADL queue.
    rebalancedPnl: The value of realised PNL that has transferred to your wallet for this position.
    prevRealisedPnl: The value of realised PNL that has transferred to your wallet for this position since the position was closed.
    currentQty: The current position amount in contracts.
    currentCost: The current cost of the position in the settlement currency of the symbol (currency).
    currentComm: The current commission of the position in the settlement currency of the symbol (currency).
    realisedCost: The realised cost of this position calculated with regard to average cost accounting.
    unrealisedCost: currentCost - realisedCost.
    grossOpenCost: The absolute value of your open orders for this symbol.
    grossOpenPremium: The amount your bidding above the mark price in the settlement currency of the symbol (currency).
    markPrice: The mark price of the symbol in quoteCurrency.
    markValue: The currentQty at the mark price in the settlement currency of the symbol (currency).
    homeNotional: Value of position in units of underlying.
    foreignNotional: Value of position in units of quoteCurrency.
    realisedPnl: The negative of realisedCost.
    unrealisedGrossPnl: markValue - unrealisedCost.
    unrealisedPnl: unrealisedGrossPnl.
    liquidationPrice: Once markPrice reaches this price, this position will be liquidated.
    bankruptPrice: Once markPrice reaches this price, this position will have no equity.
    */
    _context->orderApi()->onExecution(exec_);

    if (position->getPosState() == "Liquidation") {
        position->setPosState("Liquidated");
        position->setTimestamp(exec_->getTimestamp());
        operator<<(position);
        position->setPosState("");
        position->unsetPosState();
    }

    position->setLiquidationPrice(0.0);
    //position->setLastPrice(exec_->getLastPx());
    //position->setLastValue(position->get());
    qty_t positionDelta = (exec_->getSide() == "Buy") ? exec_->getLastQty() : - exec_->getLastQty();
    qty_t newQty = exec_->getLastQty() + position->getCurrentQty();
    qty_t oldQty = position->getCurrentQty();

    position->setAvgCostPrice((position->getAvgCostPrice() * oldQty/newQty) + (exec_->getLastPx() * exec_->getLastQty()/newQty));
    if (exec_->getSide() == "Buy") {
        position->setAvgEntryPrice((position->getAvgEntryPrice() * oldQty/newQty) + (exec_->getLastPx() * exec_->getLastQty()/newQty));
    }
    if (exec_->getSide() == "Buy")  {
        position->setExecBuyQty(exec_->getLastQty() + position->getExecBuyQty());
        position->setExecBuyCost(exec_->getExecCost() + position->getExecBuyCost());
    } else {
        position->setExecSellQty(exec_->getLastQty() + position->getExecSellQty());
        position->setExecSellCost(exec_->getExecCost() + position->getExecSellCost());
    }
    position->setExecQty(position->getExecSellQty() - position->getExecBuyQty());
    position->setExecCost(position->getExecSellCost() - position->getExecBuyCost());
    position->setExecComm(position->getExecComm() + exec_->getExecComm());
    position->setMarkValue(instrument->getMarkPrice()*position->getCurrentQty());
    position->setLastValue(position->getMarkValue());
    position->setMarkPrice(instrument->getMarkPrice());
    position->setCurrentQty(position->getCurrentQty() + positionDelta);
    position->setCurrentCost(position->getCurrentCost() + exec_->getExecCost());
    position->setUnrealisedGrossPnl(position->getMarkValue() - position->getUnrealisedCost());
    if (tradingo_utils::almost_equal(position->getCurrentQty(), 0.0)) {
        position->setRealisedPnl(position->getUnrealisedPnl());
        position->setUnrealisedPnl(0.0);
    }

    // TODO realisedCost
    // realisedCost: The realised cost of this position calculated with regard to average cost accounting.
    // unrealisedCost: currentCost - realisedCost.


    position->setCurrentComm(position->getCurrentComm() + exec_->getExecComm());

    { // set the breakeven price
        // TODO
        auto positionTotalCost = position->getAvgCostPrice()*position->getCurrentQty();
        auto bkevenPrice = position->getCurrentQty()/(positionTotalCost);
        position->setBreakEvenPrice(bkevenPrice);
    }
    { // liquidation price TODO
        auto leverageType = "ISOLATED";
        auto liqPrice = 0.0;
        position->setLiquidationPrice(liqPrice);
    }
    
    // margin
    { // gross exec cost
        price_t gross_exec_cost = 0.0;
        price_t gross_unrealised_pnl = 0.0;
        price_t gross_mark_value = 0.0;
        price_t gross_comm = 0.0;
        for (auto& pos : _context->marketData()->getPositions()) {
            gross_exec_cost += pos.second->getGrossExecCost();
            gross_unrealised_pnl += pos.second->getUnrealisedPnl();
            gross_mark_value += pos.second->getMarkValue();
            gross_comm += pos.second->getCurrentComm();
        }
        margin->setGrossExecCost(gross_exec_cost);
        margin->setUnrealisedPnl(gross_unrealised_pnl);
        margin->setGrossMarkValue(gross_mark_value);
        margin->setGrossComm(gross_comm);
    }

    double maintMargin = instrument->getMaintMargin();
    price_t exec_cost = (maintMargin * (((exec_->getSide() == "Buy") ? -1 : 1)* exec_->getExecCost())/ position->getLeverage());
    margin->setWalletBalance(margin->getAmount() + margin->getRealisedPnl() + exec_cost);
    margin->setMarginBalance(margin->getWalletBalance() + margin->getUnrealisedPnl());

}


void TestEnv::operator << (const std::shared_ptr<model::Margin>& margin_) {
    *_context->marketData() << margin_;
}


void TestEnv::operator << (const std::shared_ptr<model::Quote>& quote_) {
    *_context->marketData() << quote_;
    *_context->orderApi() >> _batchWriter;
}


void TestEnv::operator << (const std::shared_ptr<model::Instrument>& instrument_) {
    *_context->marketData() << instrument_;
}

std::shared_ptr<model::Execution> TestEnv::operator<<(const std::shared_ptr<model::Trade>& trade_) {

    auto& position = _context->marketData()->getPositions().at(trade_->getSymbol());
    auto& instrument = _context->marketData()->getInstruments().at(trade_->getSymbol());
    auto tradeQty = trade_->getSize();
    auto tradePx = trade_->getPrice();
    auto bidPrice = instrument->getBidPrice();
    auto askPrice = instrument->getAskPrice();
    for (auto& order : _context->orderApi()->orders()) {
        auto orderQty = order.second->getLeavesQty();
        auto orderPx = order.second->getPrice();
        auto side = order.second->getSide();

        if (order.second->getOrdStatus() == "Canceled"
            || order.second->getOrdStatus() == "Rejected"
            || order.second->getOrdStatus() == "Filled") {
            // order is completed
            continue;
        } else if ((side == "Buy" ? tradePx <= orderPx : tradePx >= orderPx)) {
            LOGDEBUG(AixLog::Color::GREEN << "Tradeable order found: "
                    << LOG_NVP("Order", order.second->toJson().serialize())
                    << LOG_NVP("Trade", trade_->toJson().serialize()) << AixLog::Color::none);
            auto fillQty = std::min(orderQty, tradeQty);
            auto exec = std::make_shared<model::Execution>();
            auto leavesQty = order.second->getLeavesQty() - fillQty;
            std::string liquidityIndicator = (side == "Buy" ? orderPx >= askPrice : orderPx <= bidPrice) ? 
                "RemovedLiquidity" : "AddedLiquidity";
            exec->setLastLiquidityInd(liquidityIndicator);
            exec->setSymbol(order.second->getSymbol());
            exec->setSide(side);
            exec->setOrderID(order.second->getOrderID());
            exec->setClOrdID(order.second->getClOrdID());
            exec->setAccount(1.0);
            exec->setLastPx(tradePx);
            exec->setLastQty(fillQty);
            exec->setPrice(orderPx);
            exec->setCumQty(order.second->getCumQty()+fillQty);
            exec->setLeavesQty(leavesQty);
            exec->setExecType("Trade");
            exec->setOrderQty(order.second->getOrderQty());
            exec->setTimestamp(trade_->getTimestamp());
            exec->setExecCost(func::get_cost(exec->getLastPx(), exec->getLastQty(), position->getLeverage()));
            exec->setCommission(exec->getLastLiquidityInd() == "RemovedLiquidity" ? instrument->getTakerFee() : instrument->getMakerFee());
            exec->setExecComm(exec->getCommission()*exec->getExecCost());
            LOGDEBUG(AixLog::Color::GREEN << LOG_NVP("Execution", exec->toJson().serialize()) << AixLog::Color::none);
            return exec;
        }
    }
    return nullptr;
}


struct TestAssertion {
    
    web::json::value validation_fields;
    bool fail = false;

    const char seperator    = ' ';
    const int nameWidth     = 18;
    const int numWidth      = 10;
    const std::string type;
    std::stringstream fail_message;

    TestAssertion(const std::string& type_)
    :   validation_fields(get_validation_fields())
    ,   type(type_) {

        fail_message << type << ":" << '\n' << std::left 
            << std::setw(nameWidth) << std::setfill(seperator) << "field" 
            << std::setw(numWidth) << std::setfill(seperator) << "actual" 
            << std::setw(numWidth) << std::setfill(seperator) << "expected"
            << std::endl;
    }

    void operator()(const web::json::value& actual_object, const web::json::value& expected_object) {

        auto& to_validate = validation_fields[type].as_array();

        for (auto& field : to_validate) {
            auto tolerance = field.has_double_field("tolerance") ? field["tolerance"].as_double() : 0.000001;
            auto key = field["name"].as_string();
            auto actual_num = [&key, &actual_object]() {
                return actual_object.has_double_field(key) ? actual_object.at(key).as_double() : actual_object.at(key).as_integer();
            };
            auto expected_num = [&key, &expected_object]() {
                return expected_object.has_double_field(key) ? expected_object.at(key).as_double() : expected_object.at(key).as_integer();
            };
            bool actual_is_num = actual_object.has_integer_field(key) or actual_object.has_double_field(key);
            bool expected_is_num = expected_object.has_integer_field(key) or expected_object.has_double_field(key);
            if (expected_is_num and actual_is_num) {
                auto actual = actual_num();
                auto expected = expected_num();
                if (not tradingo_utils::almost_equal(actual, expected, tolerance)) {
                    fail_message << std::left 
                        << std::setw(nameWidth) << std::setfill(seperator) << key
                        << std::setw(numWidth) << std::setfill(seperator) << actual 
                        << std::setw(numWidth) << std::setfill(seperator) << expected
                        << std::endl;
                    fail = true;
                }
            } else if (expected_object.has_string_field(key)
                    and actual_object.has_string_field(key)) {
                auto actual = actual_object.at(key).as_string();
                auto expected = expected_object.at(key).as_string();
                if (actual != expected) {
                    fail_message << std::left 
                        << std::setw(nameWidth) << std::setfill(seperator) << key
                        << std::setw(numWidth) << std::setfill(seperator) << actual 
                        << std::setw(numWidth) << std::setfill(seperator) << expected
                        << std::endl;
                    fail = true; 
                }
            } else if (not expected_object.has_field(key) and actual_object.has_field(key)) {
                fail_message << std::left 
                    << std::setw(nameWidth) << std::setfill(seperator) << key
                    << std::setw(numWidth) << std::setfill(seperator) << actual_object.at(key).as_integer()
                    << std::setw(numWidth) << std::setfill(seperator) << "Unset"
                    << std::endl;
                fail = true;
            } else if (not actual_object.has_field(key) and not expected_object.has_field(key)) {
                continue;
            } else if (not actual_object.has_field(key)) {
                fail_message << std::left 
                    << std::setw(nameWidth) << std::setfill(seperator) << key
                    << std::setw(numWidth) << std::setfill(seperator) << "Unset"
                    << std::setw(numWidth) << std::setfill(seperator) << expected_object.at(key).as_integer()
                    << std::endl;
                fail = true;
            }
        }
    }
};


void TestEnv::operator >> (const std::shared_ptr<model::Margin>& margin_) {
    auto assertMarginEqual = TestAssertion("Margin");
    auto actual_margin = _context->marketData()->getMargin()->toJson();
    auto expected_margin = margin_->toJson();
    assertMarginEqual(actual_margin, expected_margin);
    if (assertMarginEqual.fail)
        FAIL() << assertMarginEqual.fail_message.str();
    else
        LOGINFO(AixLog::Color::GREEN << "Margin: " << expected_margin.serialize() << AixLog::Color::none);
}


void TestEnv::operator >> (const std::shared_ptr<model::Position>& position_) {

    auto assertPositionEqual = TestAssertion("Position");
    auto actual_position = _context->marketData()->getPositions().at(position_->getSymbol())->toJson();
    auto expected_position = position_->toJson();
    assertPositionEqual(actual_position, expected_position);
    if (assertPositionEqual.fail)
        FAIL() << assertPositionEqual.fail_message.str();
    else
        LOGINFO(AixLog::Color::GREEN << "Position: " << expected_position.serialize() << AixLog::Color::none);
}


std::shared_ptr<model::Order> TestEnv::operator >> (const std::shared_ptr<model::Order>& order_) {
    auto& allocations = _context->strategy()->allocations();
    allocations->addAllocation(
        order_->getPrice(),
        order_->getOrderQty(),
        order_->getSide()
    );
    allocations->placeAllocations();
    auto placed_order = allocations->get(order_->getPrice())->getOrder();
    return placed_order;
}
