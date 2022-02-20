#include <gtest/gtest.h>
#include <chrono>
#include <cpprest/json.h>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#define _TURN_OFF_PLATFORM_STRING
#include "TestEnv.h"

#include <model/Margin.h>
#include "model/Execution.h"
#include "model/Trade.h"

#include "Context.h"
#include "BatchWriter.h"
#include "Series.h"


#include "Functional.h"

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
        *_context->marketData() << value_;
        if (getEventTypeFromString(value_) == "EXECUTION") {
            auto params = Params(value_);
            auto exec = std::make_shared<model::Execution>();
            auto json = params.asJson();
            exec->fromJson(json);
            _context->orderApi()->addExecToPosition(exec);
        }
        _context->strategy()->evaluate();
        // TODO make TestException class
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
    auto orderQty = order_->getLeavesQty();
    auto tradeQty = trade_->getSize();
    auto orderPx = order_->getPrice();
    auto tradePx = trade_->getPrice();
    auto side = order_->getSide();

    if (order_->getOrdStatus() == "Canceled"
        || order_->getOrdStatus() == "Rejected"
        || order_->getOrdStatus() == "Filled") {
        // order is completed
        return nullptr;
    } else if ((side == "Buy" ? tradePx <= orderPx : tradePx >= orderPx)) {
        LOGDEBUG(AixLog::Color::GREEN << "Tradeable order found: "
                << LOG_NVP("Order", order_->toJson().serialize())
                << LOG_NVP("Trade", trade_->toJson().serialize()) << AixLog::Color::none);
        auto fillQty = std::min(orderQty, tradeQty);
        auto exec = std::make_shared<model::Execution>();
        auto leavesQty = order_->getLeavesQty() - fillQty;

        exec->setSymbol(order_->getSymbol());
        exec->setSide(side);
        exec->setOrderID(order_->getOrderID());
        exec->setClOrdID(order_->getClOrdID());
        exec->setAccount(1.0);
        exec->setLastPx(tradePx);
        exec->setLastQty(fillQty);
        exec->setPrice(orderPx);
        exec->setCumQty(order_->getCumQty()+fillQty);
        exec->setLeavesQty(leavesQty);
        exec->setExecType("Trade");
        exec->setOrderQty(order_->getOrderQty());
        exec->setTimestamp(trade_->getTimestamp());
        exec->setExecCost(0.0);
        LOGDEBUG(AixLog::Color::GREEN << LOG_NVP("Execution", exec->toJson().serialize()) << AixLog::Color::none);
        return exec;
    } else {
        return nullptr;
    }

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


void TestEnv::dispatch(utility::datetime time_,
                       const std::shared_ptr<model::Quote> &quote_,
                       const std::shared_ptr<model::Execution>& exec_,
                       const std::shared_ptr<model::Order>& order_,
                       const std::shared_ptr<model::Instrument>& instrument_) {

    if (quote_) {
        LOGDEBUG(AixLog::Color::blue << "quote: " << LOG_NVP("time",time_.to_string(utility::datetime::ISO_8601)) << AixLog::Color::none);
        *_context->orderApi() << time_;
        *_context->marketData() << quote_;
        _context->strategy()->updateSignals();
    } else if (exec_ and order_) {
        LOGDEBUG(AixLog::Color::blue << "order event: " << LOG_NVP("time",time_.to_string(utility::datetime::ISO_8601)) << AixLog::Color::none);
        _context->orderApi()->addExecToPosition(exec_);
        *_context->marketData() << exec_;
        *_context->marketData() << order_;
        *_context->marketData() << _context->orderApi()->getPosition(exec_->getSymbol());
    } else if (instrument_) {
        *_context->marketData() << instrument_;
    }
}


void TestEnv::operator << (const std::shared_ptr<model::Position>& position_) {
    *_context->marketData() << position_;
}


void TestEnv::operator << (const std::shared_ptr<model::Execution>& exec_) {
    *_context->marketData() << exec_;
    auto& position = _context->marketData()->getPositions().at(exec_->getSymbol());
    auto& order = _context->orderApi()->orders().at(exec_->getClOrdID());
    auto& instrument = _context->marketData()->getInstruments().at(exec_->getSymbol());
    position->setLastPrice(exec_->getLastPx());
    position->setLastValue(position->getMarkValue());
    // TODO
    position->setLiquidationPrice(0.0);

    if (position->getPosState() == "Liquidation") {
        position->setPosState("Liquidated");
        position->setTimestamp(exec_->getTimestamp());
        operator<<(position);
        position->setPosState("");
        position->unsetPosState();
    }

    { // update the order
        order->setCumQty(order->getCumQty() + exec_->getLastQty());
        order->setLeavesQty(order->getLeavesQty() - exec_->getLastQty());
        auto newQty = order->getCumQty();
        auto oldQty = newQty - exec_->getLastQty();
        auto newAvgPx = (order->getAvgPx() * oldQty/newQty) + (exec_->getLastPx() * exec_->getLastQty()/newQty);
        order->setAvgPx(newAvgPx);
    }
    { // set avg px on position
        auto newQty = order->getCumQty() + position->getCurrentQty();
        auto oldQty = position->getCurrentQty();
        auto newAvgPx = (position->getAvgCostPrice() * oldQty/newQty) + (order->getAvgPx() * order->getCumQty()/newQty);
        position->setAvgEntryPrice(newAvgPx);
        position->setAvgCostPrice(newAvgPx);
    }
    { // update costs and quantity of position
        price_t currentCost = position->getCurrentCost();
        qty_t currentSize = position->getCurrentQty();
        price_t costDelta = func::get_cost(exec_->getLastPx(), exec_->getLastQty(), position->getLeverage());
        qty_t positionDelta = (exec_->getSide() == "Buy")
                                  ? exec_->getLastQty()
                                  : -exec_->getLastQty();
        qty_t newPosition = currentSize + positionDelta;
        price_t newCost = currentCost + costDelta;
        position->setCurrentQty(newPosition);
        position->setCurrentCost(newCost);
    }
    { // set the breakeven price
        auto positionTotalCost = position->getAvgCostPrice()*position->getCurrentQty();
        auto bkevenPrice = position->getCurrentQty()/(positionTotalCost);
        position->setBreakEvenPrice(bkevenPrice);
    }
    { // liquidation price
        auto leverageType = "ISOLATED";
        auto liqPrice = 0.0;
        position->setLiquidationPrice(liqPrice);
    }
    { // execution qty, cost of position
        position->setExecCost(position->getExecCost() + exec_->getExecCost());
        position->setExecQty(position->getExecQty() + exec_->getLastQty());
    }
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
    for (auto& order : _context->orderApi()->orders()) {
        auto exec = canTrade(order.second, trade_);
        return exec;
    }
    return nullptr;
}


void TestEnv::operator >> (const std::shared_ptr<model::Position>& position_) {

    auto validation_fields = get_validation_fields();
    auto& to_validate = validation_fields["Position"].as_array();
    auto actual_position = _context->marketData()->getPositions().at(position_->getSymbol())->toJson();
    auto expected_position = position_->toJson();
    for (auto& field : to_validate) {
        ASSERT_EQ(
            actual_position[field.to_string()],
            expected_position[field.to_string()]
        );
    }
}


void TestEnv::operator >> (const std::shared_ptr<model::Margin>& margin_) {
    auto validation_fields = get_validation_fields();
    auto& to_validate = validation_fields["Margin"].as_array();
    auto actual_margin = _context->marketData()->getMargin()->toJson();
    auto expected_margin = margin_->toJson();
    for (auto& field : to_validate) {
        ASSERT_EQ(
            actual_margin[field.to_string()],
            expected_margin[field.to_string()]
        );
    }

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
