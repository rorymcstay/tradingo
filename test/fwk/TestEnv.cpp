#include <aixlog.hpp>
#include <gtest/gtest.h>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "TestEnv.h"
#define _TURN_OFF_PLATFORM_STRING

#include <model/Margin.h>
#include <model/Execution.h>
#include <model/Instrument.h>
#include <model/Trade.h>


#include "Utils.h"
#include "Context.h"
#include "BatchWriter.h"
#include "Series.h"
#include "Functional.h"


/// get test meta file
web::json::value get_validation_fields() {
    std::string validation_fields_file =
        TESTDATA_LOCATION "/validation_fields.json";
    std::ifstream t(validation_fields_file);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string val = buffer.str();
    return web::json::value::parse(val);
}

TestEnv::TestEnv(const std::shared_ptr<Config>& config_)
    : _config(config_),
      _margin(std::make_shared<model::Margin>()),
      _positionWriter(
          "replay_positions",
          _config->get<std::string>("symbol"),
          _config->get<std::string>("storage"),
          1000,
          [](const std::shared_ptr<model::ModelBase>& order_) {
              return order_->toJson().serialize();
          },
          /*rotate=*/false),
      _batchWriter(
          "replay_orders",
          _config->get<std::string>("symbol"),
          _config->get<std::string>("storage"), 1,
          [](const std::shared_ptr<model::ModelBase>& order_) {
              return order_->toJson().serialize();
          },
          /*rotate=*/false) {
    init();
}

TestEnv::TestEnv(
    std::initializer_list<std::pair<std::string, std::string>> config_)
    : _config(std::make_shared<Config>(config_)),
      _margin(std::make_shared<model::Margin>()),
      _positionWriter(
          "replay_positions",
          _config->get<std::string>("symbol"),
          _config->get<std::string>("storage"),
          1000,
          [](const std::shared_ptr<model::ModelBase>& order_) {
              return order_->toJson().serialize();
          },
          /*rotate=*/false),
      _batchWriter(
          "replay_orders",
          _config->get<std::string>("symbol"),
          _config->get<std::string>("storage"), 1,
          [](const std::shared_ptr<model::ModelBase>& order_) {
              return order_->toJson().serialize();
          },
          /*rotate=*/false) {
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

    _context =
        std::make_shared<Context<TestMarketData, OrderApi, TestPositionApi>>(
            _config);

    _context->orderApi()->setMarketData(_context->marketData());
    _context->positionApi()->setMarketData(_context->marketData());

    // initialise strategy as the very last thing we do
    _context->init();
    _context->initStrategy();

}

void TestEnv::operator<<(const std::string& value_) {
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
        FAIL() << "TEST Exception: " << ex.what()
               << " during event <<\n\n      " << value_;
    }
}

std::shared_ptr<model::Order> TestEnv::operator>>(const std::string& value_) {
    try {
        *_context->orderApi() >> value_;
        auto eventType = getEventTypeFromString(value_);
        return _context->orderApi()->getEvent(eventType);
    } catch (std::runtime_error& ex) {
        std::stringstream error_msg;
        error_msg << "TEST Exception: " << ex.what()
                  << " during event >>\n\n      " << value_;
        // FAIL() << error_msg;
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

#define LOG_STATS(stats_)                                                      \
    std::stringstream positions_sstr;                                          \
    for (auto& pos : _context->marketData()->getPositions()) {                 \
        positions_sstr << pos.first                                            \
            << "=" <<  pos.second->toJson().serialize() << ", ";               \
    }                                                                          \
    double quotes_progress =                                                   \
        100 * (stats_.quotes_processed / (double)stats_.quotes_size);          \
    double instruments_progress =                                              \
        100 * (stats.instruments_processed / (double)stats_.instruments_size); \
    double trades_progress =                                                   \
        100 * (stats.trades_processed / (double)stats_.trades_size);           \
    LOGWARN(                                                                   \
        "STATS: "                                                              \
        << LOG_NVP("QuoteProgress", quotes_progress)                           \
        << LOG_NVP("TradeProgress", trades_progress)                           \
        << LOG_NVP("InstrumentProgress", instruments_progress)                 \
        << LOG_NVP("StartTime", stats_.start_time.to_string(                   \
                                    utility::datetime::date_format::ISO_8601)) \
        << LOG_NVP("CurrentTime",                                              \
                   stats_.current_time.to_string(                              \
                       utility::datetime::date_format::ISO_8601))              \
        << LOG_NVP("EndTime", stats_.end_time.to_string(                       \
                                  utility::datetime::date_format::ISO_8601))   \
        << LOG_NVP("Positions", positions_sstr.str()));

void TestEnv::playback(const Series<model::Trade>& trades,
                       const Series<model::Quote>& quotes,
                       const Series<model::Instrument>& instruments) {

    auto quote = quotes.begin();
    auto trade = trades.begin();
    auto instrument = instruments.begin();

    auto stats = PlaybackStats{quotes.begin()->getTimestamp(),
                               quotes.end()->getTimestamp(),
                               quotes.begin()->getTimestamp(),
                               quotes.size(),
                               trades.size(),
                               instruments.size(),
                               };

    std::vector<std::shared_ptr<model::ModelBase>> outBuffer;

    auto symbol = _config->get<std::string>("symbol");

    utility::datetime::interval_type max_interval =
        std::numeric_limits<utility::datetime::interval_type>::max();

    while (quote != quotes.end() or trade != trades.end() or
           instrument != instruments.end()) {

        utility::datetime::interval_type current_time = std::min(
            {(quote != quotes.end()) ? quote->getTimestamp().to_interval()
                                     : max_interval,
             (trade != trades.end()) ? trade->getTimestamp().to_interval()
                                     : max_interval,
             (instrument != instruments.end())
                 ? instrument->getTimestamp().to_interval()
                 : max_interval});
        // TRADE
        if (trade != trades.end() and
            trade->getTimestamp().to_interval() == current_time) {
            *(_context->orderApi()) << trade->getTimestamp();
            auto exec = operator<<(*trade);
            if (exec) {
                LOGINFO("Simulated execution");
                operator<<(exec);
            }
            ++trade;
            stats.trades_processed += 1;
            stats.current_time = trade->getTimestamp();
            // QUOTE
        } else if (quote != quotes.end() and
                   quote->getTimestamp().to_interval() == current_time) {
            *(_context->orderApi()) << quote->getTimestamp();
            operator<<(*quote);
            ++quote;
            stats.quotes_processed += 1;
            stats.current_time = quote->getTimestamp();
            // INSTRUMENT
        } else if (instrument != instruments.end() and
                   instrument->getTimestamp().to_interval() == current_time) {
            *(_context->orderApi()) << instrument->getTimestamp();
            auto position = _context->marketData()->getPositions().at(
                instrument->getSymbol());
            if ((position->getLiquidationPrice() > 0.0) and
                ((position->getCurrentQty() > 0.0 &&
                  instrument->getMarkPrice() <=
                      position->getLiquidationPrice()) or
                 (position->getCurrentQty() < 0.0 &&
                  instrument->getMarkPrice() >= position->getMarkPrice()))) {
                liquidatePositions(position->getSymbol());
            }
            operator<<(*instrument);
            ++instrument;
            stats.instruments_processed += 1;
            stats.current_time = instrument->getTimestamp();
            // Something has finished.
        } else {
            std::stringstream ss;
            if (not(trade != trades.end())) {
                ss << "Trades ";
            } else if (not(quote != quotes.end())) {
                ss << "Quotes ";
            } else if (not(instrument != instruments.end())) {
                ss << "Instruments ";
            } else {
                ss << "UNKNOWN ";
            }
            ss << "Has completed.";
            LOGWARN(ss.str());
        }
        // STAT interval log
        if (std::chrono::system_clock::now() - stats.last_log >
            stats.interval) {
            LOG_STATS(stats);
            stats.last_log = std::chrono::system_clock::now();
        }
    }
    _batchWriter.write_batch();
    _positionWriter.write_batch();
}

void TestEnv::liquidatePositions(const std::string& symbol) {

    auto md = _context->strategy()->getMD();

    auto position = md->getPositions().at(symbol);

    auto qty = position->getCurrentQty();
    auto margin = md->getMargin();
    auto liqPrice = position->getLiquidationPrice();
    auto markPrice = position->getMarkPrice();

    position->setPosState("Liquidation");
    auto execution = std::make_shared<model::Execution>();
    execution->setOrderID("LIQUIDATION");
    execution->setLeavesQty(0.0);
    execution->setLastPx(position->getLiquidationPrice());
    execution->setLastQty(std::abs(position->getCurrentQty()));
    execution->setSide(position->getCurrentQty() >= 0.0 ? "Sell" : "Buy");
    execution->setAvgPx(position->getLiquidationPrice());
    execution->setCumQty(position->getCurrentQty());
    execution->setText("Liquidation");
    execution->setLastLiquidityInd("RemovedLiquidity");
    execution->setOrderQty(std::abs(position->getCurrentQty()));
    execution->setSymbol(symbol);
    auto cost = ((execution->getSide() == "Buy") ? -1 : 1) *
                func::get_cost(execution->getLastPx(), execution->getLastQty(),
                               position->getLeverage());
    execution->setExecCost(cost);

    LOGINFO("Auto liquidation triggered: "
            << LOG_VAR(liqPrice) << LOG_VAR(markPrice) << LOG_VAR(qty)
            << LOG_NVP("balance", margin->getWalletBalance()));
    operator<<(execution);
}

void TestEnv::operator<<(const std::shared_ptr<model::Position>& position_) {
    *_context->marketData() << position_;
}

void TestEnv::operator<<(const std::shared_ptr<model::Execution>& exec_) {

    auto& position =
        _context->marketData()->getPositions().at(exec_->getSymbol());
    auto& instrument =
        _context->marketData()->getInstruments().at(exec_->getSymbol());
    auto& margin = _context->marketData()->getMargin();

    /*
        account: Your unique account ID.
        symbol: The contract for this position.
        currency: The margin currency for this position.
        underlying: Meta data of the symbol.
        quoteCurrency: Meta data of the symbol, All prices are in the quoteCurrency
        commission: The maximum of the maker, taker, and settlement fee.
        initMarginReq: The initial margin requirement. This will be at least the
            symbol's default initial maintenance margin, but can be higher if you choose
            lower leverage.
        maintMarginReq: The maintenance margin requirement. This will be at least
            the symbol's default maintenance maintenance margin, but  can be higher
            if you choose a higher risk limit.
        riskLimit: This is a function of your maintMarginReq.
        leverage: 1 / initMarginReq.
        crossMargin: True/false depending on whether you set cross margin on this
            position.
        deleveragePercentile: Indicates where your position is in the ADL queue.
        rebalancedPnl: The value of realised PNL that has transferred to your wallet
            for this position.
        prevRealisedPnl: The value of realised PNL that has transferred to your
            wallet for this position since the position was closed.
        currentQty: The current position amount in contracts.
        currentCost: The current cost of the position in the settlement currency of
            the symbol (currency).
        currentComm: The current commission of the position in the settlement currency
            of the symbol (currency).
        realisedCost: The realised cost of this position calculated with regard to
            average cost accounting.
        unrealisedCost: currentCost - realisedCost. gr absolute value of your open orders
            for this symbol.
        grossOpenPremium: The amount your bidding above the mark price in the settlement
            currency of the symbol (currency).
        markPrice: The mark price of the symbol in quoteCurrency.
        markValue: The currentQty at the mark price in the settlement currency of the
            symbol (currency).
        homeNotional: Value of position in units of underlying.
        foreignNotional: Value of position in units of quoteCurrency.
        realisedPnl: The negative of realisedCost.
        unrealisedGrossPnl: markValue - unrealisedCost.
        unrealisedPnl: unrealisedGrossPnl.
        liquidationPrice: Once markPrice reaches this price, this position will be
            liquidated.
        bankruptPrice: Once markPrice reaches this price, this position will have
            no equity.
    */

    if (position->getPosState() == "Liquidation") {
        position->setPosState("Liquidated");
        position->setTimestamp(exec_->getTimestamp());
        operator<<(position);
        position->setPosState("");
        position->unsetPosState();
    }

    // TODO
    position->setLiquidationPrice(0.0);

    if (exec_->getExecType() == "Trade") {
        position->setTimestamp(exec_->getTimestamp());
        qty_t positionDelta = (exec_->getSide() == "Buy")
                                  ? exec_->getLastQty()
                                  : -exec_->getLastQty();
        qty_t newQty = exec_->getLastQty() + position->getCurrentQty();
        qty_t oldQty = position->getCurrentQty();

        position->setCurrentQty(position->getCurrentQty() + positionDelta);
        position->setCurrentCost(position->getCurrentCost() +
                                 exec_->getExecCost());
        position->setAvgCostPrice(
            (position->getAvgCostPrice() * oldQty / newQty) +
            (exec_->getLastPx() * exec_->getLastQty() / newQty));

        position->setUnrealisedCost(position->getCurrentCost() -
                                    position->getRealisedCost());
        position->setRealisedPnl(position->getRealisedPnl() +
                                 exec_->getExecComm());

        if (exec_->getSide() == "Buy") {
            position->setExecBuyQty(exec_->getLastQty() +
                                    position->getExecBuyQty());
            position->setExecBuyCost(std::abs(exec_->getExecCost()) +
                                     position->getExecBuyCost());
            position->setAvgEntryPrice(
                (position->getAvgEntryPrice() * oldQty / newQty) +
                (exec_->getLastPx() * exec_->getLastQty() / newQty));
        } else {
            position->setExecSellQty(exec_->getLastQty() +
                                     position->getExecSellQty());
            position->setExecSellCost(exec_->getExecCost() +
                                      position->getExecSellCost());
        }
        position->setExecQty(position->getExecBuyQty() -
                             position->getExecSellQty());
        position->setExecCost(position->getExecSellCost() -
                              position->getExecBuyCost());
        position->setExecComm(position->getExecComm() + exec_->getExecComm());

        position->setCurrentComm(
            position->getCurrentComm() +
            exec_->getExecComm()); // commission arising from funding +
                                   // executions
        price_t exec_cost =
            (instrument->getMaintMargin() *
             (((exec_->getSide() == "Buy") ? -1 : 1) * exec_->getExecCost()) /
             position->getLeverage());
        margin->setWalletBalance(margin->getAmount() +
                                 margin->getRealisedPnl() + exec_cost);
        margin->setMarginBalance(margin->getWalletBalance() +
                                 margin->getUnrealisedPnl());
    }
    updatePositionFromInstrument(instrument);

    // realisedCost
    // realisedCost: The realised cost of this position calculated with regard
    // to average cost accounting. unrealisedCost: currentCost - realisedCost.
    if (position->getUnrealisedPnl() != 0.0 and
        tradingo_utils::almost_equal(position->getCurrentQty(), 0.0)) {
        position->setRealisedPnl(position->getRealisedPnl() +
                                 position->getUnrealisedPnl());
        position->setRealisedCost(position->getRealisedCost() +
                                  position->getUnrealisedCost());
        position->setUnrealisedPnl(0.0);
        position->setUnrealisedCost(0.0);
        position->unsetBreakEvenPrice();
        position->unsetLiquidationPrice();
    }

    if (_context->orderApi()->orders().find(exec_->getClOrdID()) 
            != _context->orderApi()->orders().end()) {
        auto& order = _context->orderApi()->orders().at(exec_->getClOrdID());
        *_context->marketData() << exec_;
        *_context->marketData() << order; // deletes the order from market data
        *_context->orderApi() << exec_;
    } else {
        FAIL() << AixLog::Color::RED
            << LOG_NVP("ClOrdID", exec_->getClOrdID())
            << "Not found" << AixLog::Color::NONE;
    }
}

void TestEnv::updatePositionFromInstrument(
    const std::shared_ptr<model::Instrument>& instrument) {
    auto& position =
        _context->marketData()->getPositions().at(instrument->getSymbol());
    auto& margin = _context->marketData()->getMargin();
    auto old_mark_value = position->getMarkValue();
    position->setMarkValue((position->getCurrentQty() >= 0.0 ? -1 : 1) *
                           func::get_cost(instrument->getMarkPrice(),
                                          position->getCurrentQty(), 1.0));
    position->setUnrealisedPnl(position->getMarkValue() -
                               position->getUnrealisedCost());
    position->setLastValue(position->getMarkValue());
    position->setMarkPrice(instrument->getMarkPrice());
    position->setMaintMargin(
        (1 + instrument->getMaintMargin()) *
            (position->getMarkValue() / position->getLeverage()) +
        position->getUnrealisedPnl());

    position->setUnrealisedGrossPnl(position->getMarkValue() -
                                    position->getUnrealisedCost());
    position->setUnrealisedCost(position->getCurrentCost() -
                                position->getRealisedCost());

    { // set the breakeven price
        // TODO
        auto positionTotalCost =
            position->getAvgCostPrice() * position->getCurrentQty();
        auto bkevenPrice = position->getCurrentQty() / (positionTotalCost);
        position->setBreakEvenPrice(bkevenPrice);
    }
    // margin
    { // gross exec cost
        price_t gross_exec_cost = 0.0;
        price_t gross_unrealised_pnl = 0.0;
        price_t gross_mark_value = 0.0;
        price_t gross_comm = 0.0;
        price_t gross_realised_pnl = 0.0;
        price_t gross_maint_margin = 0.0;
        for (auto& pos : _context->marketData()->getPositions()) {
            gross_exec_cost += pos.second->getGrossExecCost();
            gross_unrealised_pnl += pos.second->getUnrealisedPnl();
            gross_mark_value += std::abs(pos.second->getMarkValue());
            gross_comm += pos.second->getCurrentComm();
            gross_realised_pnl += pos.second->getRealisedPnl();
            gross_maint_margin += pos.second->getMaintMargin();
        }
        margin->setGrossExecCost(gross_exec_cost);
        margin->setUnrealisedPnl(gross_unrealised_pnl);
        margin->setGrossMarkValue(gross_mark_value);
        margin->setGrossComm(gross_comm);
        margin->setRealisedPnl(gross_realised_pnl);
    }

    if (not tradingo_utils::almost_equal(position->getMarkValue(),
                                         old_mark_value)) {
        auto position_event = std::make_shared<model::Position>();
        auto pos_json = position->toJson();
        position_event->fromJson(pos_json);
        _positionWriter.write(position);
    }
}

void TestEnv::operator<<(const std::shared_ptr<model::Margin>& margin_) {
    *_context->marketData() << margin_;
}

void TestEnv::operator<<(const std::shared_ptr<model::Quote>& quote_) {
    *_context->marketData() << quote_;
    *_context->orderApi() >> _batchWriter;
}

void TestEnv::operator<<(
    const std::shared_ptr<model::Instrument>& instrument_) {
    *_context->marketData() << instrument_;
    auto instrument =
        _context->marketData()->getInstruments().at(instrument_->getSymbol());
    _context->marketData()->getPositions().at(
            instrument->getSymbol())->setTimestamp(instrument_->getTimestamp());
    updatePositionFromInstrument(instrument);
}

std::shared_ptr<model::Execution>
TestEnv::operator<<(const std::shared_ptr<model::Trade>& trade_) {

    auto& position =
        _context->marketData()->getPositions().at(trade_->getSymbol());
    auto& instrument =
        _context->marketData()->getInstruments().at(trade_->getSymbol());
    auto tradeQty = trade_->getSize();
    auto tradePx = trade_->getPrice();
    auto bidPrice = instrument->getBidPrice();
    auto askPrice = instrument->getAskPrice();
    for (auto& order : _context->orderApi()->orders()) {
        auto orderQty = order.second->getLeavesQty();
        auto orderPx = order.second->getPrice();
        auto side = order.second->getSide();

        if (order.second->getOrdStatus() == "Canceled" ||
            order.second->getOrdStatus() == "Rejected" ||
            order.second->getOrdStatus() == "Filled" ||
            tradingo_utils::almost_equal(orderQty, 0.0)) {
            // order is completed
            continue;
        } else if ((side == "Buy" ? tradePx <= orderPx : tradePx >= orderPx)) {

            auto fillQty = std::min(orderQty, tradeQty);
            auto exec = std::make_shared<model::Execution>();
            auto leavesQty = order.second->getLeavesQty() - fillQty;
            std::string liquidityIndicator =
                (side == "Buy" ? orderPx >= askPrice : orderPx <= bidPrice)
                    ? "RemovedLiquidity"
                    : "AddedLiquidity";
            exec->setExecType("Trade");
            exec->setLastLiquidityInd(liquidityIndicator);
            exec->setSymbol(order.second->getSymbol());
            exec->setSide(side);
            exec->setOrderID(order.second->getOrderID());
            exec->setClOrdID(order.first);
            exec->setLastPx(tradePx);
            exec->setAccount(order.second->getAccount());
            exec->setLastQty(fillQty);
            exec->setPrice(orderPx);
            exec->setCumQty(order.second->getCumQty() + fillQty);
            exec->setOrderQty(order.second->getOrderQty());
            exec->setTimeInForce(order.second->getTimeInForce());
            exec->setOrdType(order.second->getOrdType());
            exec->setExDestination(order.second->getExDestination());
            exec->setTrdMatchID(trade_->getTrdMatchID());
            exec->setLeavesQty(order.second->getLeavesQty() -
                               exec->getLastQty());
            exec->setLastMkt("XSIM");
            if (tradingo_utils::almost_equal(exec->getLeavesQty(), 0.0)) {
                exec->setOrdStatus("Filled");
            } else {
                exec->setOrdStatus("PartiallyFilled");
            }

            auto cost = ((exec->getSide() == "Buy") ? -1 : 1) *
                        func::get_cost(exec->getLastPx(), exec->getLastQty(),
                                       position->getLeverage());
            exec->setExecCost(cost);
            exec->setCommission(exec->getLastLiquidityInd() ==
                                        "RemovedLiquidity"
                                    ? instrument->getTakerFee()
                                    : instrument->getMakerFee());
            exec->setExecComm(exec->getCommission() *
                              std::abs(exec->getExecCost()));
            LOGINFO(AixLog::Color::GREEN
                    << "Tradeable order found: "
                    << LOG_NVP("Order", order.second->toJson().serialize())
                    << LOG_NVP("Execution", exec->toJson().serialize())
                    << AixLog::Color::NONE);
            return exec;
        }
    }
    return nullptr;
}

struct TestAssertion {

    web::json::value validation_fields;
    bool fail = false;

    const char seperator = ' ';
    const int nameWidth = 18;
    const int numWidth = 20;
    const std::string type;
    std::stringstream fail_message;

    TestAssertion(const std::string& type_)
        : validation_fields(get_validation_fields()), type(type_) {

        fail_message << type << ":" << '\n'
                     << std::left << std::setw(nameWidth)
                     << std::setfill(seperator) << "field"
                     << std::setw(numWidth) << std::setfill(seperator)
                     << "actual" << std::setw(numWidth)
                     << std::setfill(seperator) << "expected" << std::endl;
    }

    void operator()(const web::json::value& actual_object,
                    const web::json::value& expected_object) {

        auto& to_validate = validation_fields[type].as_array();

        for (auto& field : to_validate) {
            bool field_failure = false;
            auto tolerance = field.has_field("tolerance")
                                 ? field["tolerance"].as_double()
                                 : 0.000001;
            auto key = field["name"].as_string();
            auto actual_num = [&key, &actual_object]() {
                return actual_object.has_double_field(key)
                           ? actual_object.at(key).as_double()
                           : actual_object.at(key).as_integer();
            };
            auto expected_num = [&key, &expected_object]() {
                return expected_object.has_double_field(key)
                           ? expected_object.at(key).as_double()
                           : expected_object.at(key).as_integer();
            };
            bool actual_is_num = actual_object.has_integer_field(key) or
                                 actual_object.has_double_field(key);
            bool expected_is_num = expected_object.has_integer_field(key) or
                                   expected_object.has_double_field(key);
            if (expected_is_num and actual_is_num) {
                auto actual = actual_num();
                auto expected = expected_num();
                if (not tradingo_utils::almost_equal(actual, expected,
                                                     tolerance)) {
                    fail_message
                        << std::left << AixLog::Color::RED
                        << std::setw(nameWidth) << std::setfill(seperator)
                        << key << std::setw(numWidth) << std::setfill(seperator)
                        << actual << std::setw(numWidth)
                        << std::setfill(seperator) << expected
                        << AixLog::Color::NONE << std::endl;
                    field_failure = true;
                }
            } else if (expected_object.has_string_field(key) and
                       actual_object.has_string_field(key)) {
                auto actual = actual_object.at(key).as_string();
                auto expected = expected_object.at(key).as_string();
                if (actual != expected) {
                    fail_message
                        << std::left << AixLog::Color::RED
                        << std::setw(nameWidth) << std::setfill(seperator)
                        << key << std::setw(numWidth) << std::setfill(seperator)
                        << actual << std::setw(numWidth)
                        << std::setfill(seperator) << expected
                        << AixLog::Color::NONE << std::endl;
                    field_failure = true;
                }
            } else if (not expected_object.has_field(key) and
                       actual_object.has_field(key)) {
                fail_message
                    << std::left << AixLog::Color::RED << std::setw(nameWidth)
                    << std::setfill(seperator) << key << std::setw(numWidth)
                    << std::setfill(seperator)
                    << actual_object.at(key).as_integer() << std::setw(numWidth)
                    << std::setfill(seperator) << "Unset" << AixLog::Color::NONE
                    << std::endl;
                field_failure = true;
            } else if (not actual_object.has_field(key) and
                       not expected_object.has_field(key)) {
                continue;
            } else if (not actual_object.has_field(key)) {
                fail_message
                    << std::left << AixLog::Color::RED << std::setw(nameWidth)
                    << std::setfill(seperator) << key << std::setw(numWidth)
                    << std::setfill(seperator) << "Unset" << std::setw(numWidth)
                    << std::setfill(seperator)
                    << expected_object.at(key).as_integer()
                    << AixLog::Color::NONE << std::endl;
                field_failure = true;
            }
            if (not field_failure) {
                fail_message << std::left << AixLog::Color::GREEN
                             << std::setw(nameWidth) << std::setfill(seperator)
                             << key;
                if (actual_is_num) {
                    fail_message
                        << std::setw(numWidth) << std::setfill(seperator)
                        << actual_object.at(key).as_integer()
                        << std::setw(numWidth) << std::setfill(seperator)
                        << expected_object.at(key).as_integer();
                } else {
                    fail_message
                        << std::setw(numWidth) << std::setfill(seperator)
                        << actual_object.at(key).as_string()
                        << std::setw(numWidth) << std::setfill(seperator)
                        << expected_object.at(key).as_string();
                }
                fail_message << AixLog::Color::NONE << std::endl;
            } else {
                fail = true;
            }
        }
    }
};

void TestEnv::operator>>(const std::shared_ptr<model::Margin>& margin_) {
    auto assertMarginEqual = TestAssertion("Margin");
    auto actual_margin = _context->marketData()->getMargin()->toJson();
    auto expected_margin = margin_->toJson();
    assertMarginEqual(actual_margin, expected_margin);
    if (assertMarginEqual.fail)
        FAIL() << assertMarginEqual.fail_message.str();
    else
        LOGINFO("Margin: " << assertMarginEqual.fail_message.str()
                           << AixLog::Color::none);
}

void TestEnv::operator>>(const std::shared_ptr<model::Position>& position_) {

    auto assertPositionEqual = TestAssertion("Position");
    auto actual_position = _context->marketData()
                               ->getPositions()
                               .at(position_->getSymbol())
                               ->toJson();
    auto expected_position = position_->toJson();
    assertPositionEqual(actual_position, expected_position);
    if (assertPositionEqual.fail)
        FAIL() << assertPositionEqual.fail_message.str();
    else
        LOGINFO(AixLog::Color::GREEN
                << "Position: " << assertPositionEqual.fail_message.str()
                << AixLog::Color::none);
}

std::shared_ptr<model::Order>
TestEnv::operator>>(const std::shared_ptr<model::Order>& order_) {
    auto& allocations = _context->strategy()->allocations();
    allocations->addAllocation(order_->getPrice(), order_->getOrderQty(),
                               order_->getSide());
    allocations->placeAllocations();
    auto placed_order = allocations->get(order_->getPrice())->getOrder();

    return placed_order;
}
