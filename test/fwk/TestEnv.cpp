#define _TURN_OFF_PLATFORM_STRING
#include <Context.h>
#include <BatchWriter.h>
#include "TestEnv.h"


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _config(std::make_shared<Config>(config_))
,   _position(std::make_shared<model::Position>())
,   _margin(std::make_shared<model::Margin>())
,   _lastDispatch()
,   _realtime(false)
,   _events(0)
,   _marginCalculator(std::make_shared<MarginCalculator>(_config))
{
    init();
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *_context->marketData() << value_;
        if (_realtime) {
            sleep(_context->marketData()->time());
            _lastDispatch.actual_time = utility::datetime::utc_now();
            _lastDispatch.mkt_time = _context->marketData()->time();
        }
        _events++;
        _context->strategy()->evaluate();
        // TODO make TestException class
    } catch (std::exception& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event <<\n\n      " << value_;
    }
}

std::shared_ptr<model::Order> TestEnv::operator>>(const std::string &value_) {
    try {
        auto order = *_context->orderApi() >> value_;
        return order;
    } catch (std::runtime_error& ex) {
        std::stringstream error_msg;
        error_msg << "TEST Exception: " << ex.what() << " during event >>\n\n      " << value_;
        //FAIL() << error_msg;
        throw ex;
    }
}

template<typename T>
std::shared_ptr<T> getEvent(std::ifstream &fileHandle_) {
    std::string str;
    auto quote = std::make_shared<T>();
    if (not std::getline(fileHandle_, str))
        return nullptr;
    if (str.empty())
        return getEvent<T>(fileHandle_);
    auto json = web::json::value::parse(str);
    quote->fromJson(json);
    return quote;
}

/// if trade may not occur, return nullptr, else return exec report and modify order.
std::shared_ptr<model::Execution> canTrade(const std::shared_ptr<model::Order>& order_, const std::shared_ptr<model::Trade>& trade_) {
    auto orderQty = order_->getLeavesQty();
    auto tradeQty = trade_->getSize();
    auto orderPx = order_->getPrice();
    auto tradePx = trade_->getPrice();
    auto side = order_->getSide();

    if (order_->getOrdStatus() == "Canceled"
        || order_->getOrdStatus() == "Rejected"
        || order_->getOrdStatus() == "Filled") {
        return nullptr;
        // order is filled, do nothing
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
        if (almost_equal(leavesQty, 0.0)) {
            order_->setOrdStatus("Filled");
        } else {
            order_->setOrdStatus("PartiallyFilled");
        }
        auto cumQty = order_->getCumQty() + fillQty;
        order_->setLeavesQty(leavesQty);
        order_->setCumQty(cumQty);
        LOGDEBUG(AixLog::Color::GREEN << LOG_NVP("Order", order_->toJson().serialize()) << AixLog::Color::none);

        return exec;
    } else {
        return nullptr;
    }

}

void TestEnv::playback(const std::string& tradeFile_, const std::string& quoteFile_) {
    std::ifstream tradeFile;
    std::ifstream quoteFile;
    tradeFile.open(tradeFile_);
    quoteFile.open(quoteFile_);
    if (not tradeFile.is_open()) {
        std::stringstream msg;
        msg << "File " << LOG_VAR(tradeFile_) << " does not exist.";
        throw std::runtime_error(msg.str());
    }
    if (not quoteFile.is_open()) {
        std::stringstream msg;
        msg << "File " << LOG_VAR(tradeFile_) << " does not exist.";
        throw std::runtime_error(msg.str());
    }
    bool stop = false;
    auto quote = getEvent<model::Quote>(quoteFile);
    auto trade = getEvent<model::Trade>(tradeFile);
    bool hasTrades = true;

    std::vector<std::shared_ptr<model::ModelBase>> outBuffer;
    // table writers
    auto printer = [](const std::shared_ptr<model::ModelBase>& order_) {
        return order_->toJson().serialize();
    };
    auto batchWriter = TestOrdersApi::Writer("replay_orders",
                                             _context->config()->get("symbol"),
                                             _context->config()->get("storage"), 5,
                                             printer, /*rotate=*/false);
    auto positionWriter = TestOrdersApi::Writer("replay_positions",
                                                _context->config()->get("symbol"),
                                                _context->config()->get("storage"), 5,
                                                printer, /*rotate=*/false);
    _lastDispatch.mkt_time = quote->getTimestamp();
    while (not stop) {
        if (!hasTrades or trade->getTimestamp() >= quote->getTimestamp()) {
            // trades are ahead of quotes, send the quote
            (*_context->orderApi()->getMarginCalculator())(quote);
            auto time = quote->getTimestamp();
            // QUOTE
            // TODO: env << "EXEC Price=..."
            dispatch(time, quote, nullptr, nullptr);
            // set the quote for next iteration.
            quote = getEvent<model::Quote>(quoteFile);
            // record replay actions to a file.
            *_context->orderApi() >> batchWriter;

            if (not quote) {
                stop = true;
            }

            // TODO: void liquidatePositions(positions)
            {
                std::shared_ptr<MarginCalculator> mc = _context->orderApi()->getMarginCalculator();
                auto md = _context->strategy()->getMD();
                auto position = md->getPositions().at(_config->get("symbol"));
                auto liqPrice = position->getLiquidationPrice();
                auto qty = position->getCurrentQty();
                auto margin = md->getMargin();


                if ((qty >= 0 and mc->getMarkPrice() > liqPrice)
                    or (qty < 0 and mc->getMarkPrice() < liqPrice)) {

                    position->setCurrentQty(0);
                    position->setUnrealisedPnl(-1);
                    auto cost_to_balance = position->getCurrentCost();
                    if (_config->get("leverageType", "ISOLATED") == "CROSSED")
                        margin->setWalletBalance(margin->getWalletBalance() - cost_to_balance);
                    positionWriter.write(position);
                    LOGINFO("Auto liquidation triggered: "
                                    << LOG_VAR(liqPrice)
                                    << LOG_VAR(qty)
                                    << LOG_NVP("markPrice", mc->getMarkPrice())
                                    << LOG_NVP("balance", margin->getWalletBalance()));
                    *(_context->marketData()) << position;
                }
            }
        } else { // send the trade.
            // TODO Check if trade can match on what we have? then send EXECUTION
            for (auto& order : _context->orderApi()->orders()) {

                // TODO: env << "EXEC Price=..."
                auto exec = canTrade(order.second, trade);
                if (exec) {
                    _context->orderApi()->set_order_timestamp(order.second);
                    exec->setTimestamp(order.second->getTimestamp());
                    // put resultant execution into marketData.
                    LOGINFO(LOG_NVP("OrderID", order.second->getOrderID())
                        << AixLog::Color::GREEN
                        << LOG_NVP("Side", order.second->getSide())
                        << LOG_NVP("CumQty", order.second->getCumQty())
                        << LOG_NVP("LeavesQty", order.second->getLeavesQty())
                        << LOG_NVP("OrdStatus", order.second->getOrdStatus())
                        << AixLog::Color::none);
                dispatch(exec->getTimestamp(), nullptr, exec, order.second);
                if (exec->getOrdStatus() == "Filled") {
                    LOGDEBUG("Filled order");
                }
                positionWriter.write(_context->orderApi()->getPosition());
            }
        }
        _context->strategy()->evaluate();

        trade = getEvent<model::Trade>(tradeFile);
        if (not trade) {
            // trades are finished now
            hasTrades = false;
        }
    }

}
batchWriter.write_batch();
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
    if (_config->get("realtime", "false") == "true") {
        _realtime = true;
    }
    if (_config->get("logLevel", "").empty())
        _config->set("logLevel", "debug");
    _config->set("cloidSeed", "0");
    auto tickSize = std::atof(_config->get("tickSize", "0.5").c_str());
    auto referencePrice = std::atof(_config->get("referencePrice").c_str());
    auto lotSize = std::atof(_config->get("lotSize", "100").c_str());
    auto instSvc = std::shared_ptr<InstrumentService>(nullptr);
    _context = std::make_shared<Context<TestMarketData, OrderApi, TestPositionApi>>(_config);
    auto instrument = std::make_shared<model::Instrument>();
    instrument->setSymbol(_config->get("symbol"));
    instrument->setPrevPrice24h(referencePrice);
    instrument->setTickSize(tickSize);
    instrument->setLotSize(lotSize);
    _context->instrumentService()->add(instrument);
    _context->init();
    _context->initStrategy();
    _position->setSymbol(_config->get("symbol"));
    _context->orderApi()->setMarginCalculator(_marginCalculator);
    _margin->setWalletBalance(std::atof(_config->get("startingBalance").c_str()));
    _margin->setCurrency("XBt");
    _context->orderApi()->setPosition(_position);
    _context->orderApi()->setMargin(_margin);
    _context->marketData()->addPosition(_position);
    _context->orderApi()->init(_config);

}

TestEnv::TestEnv(const std::shared_ptr<Config> &config_)
:   _config(config_)
,   _position(std::make_shared<model::Position>())
,   _realtime(false){
init();
}


void TestEnv::dispatch(utility::datetime time_, const std::shared_ptr<model::Quote> &quote_,
                       const std::shared_ptr<model::Execution> exec_, const std::shared_ptr<model::Order> order_) {

    // default behaviour is not to sleep
    if (_realtime) {
        sleep(time_);
    }
    _events++;
    _lastDispatch.actual_time = utility::datetime::utc_now();
    if (quote_) {
        _lastDispatch.mkt_time = quote_->getTimestamp();
        LOGDEBUG(AixLog::Color::blue << "quote: " << LOG_NVP("time",time_.to_string(utility::datetime::ISO_8601)) << AixLog::Color::none);
        *_context->orderApi() << time_;
        *_context->marketData() << quote_;
        _context->strategy()->updateSignals();
        _context->strategy()->evaluate();
    } else if (exec_ and order_) {
        LOGDEBUG(AixLog::Color::blue << "order event: " << LOG_NVP("time",time_.to_string(utility::datetime::ISO_8601)) << AixLog::Color::none);
        _context->orderApi()->addExecToPosition(exec_);
        *_context->marketData() << exec_;
        *_context->marketData() << order_;
        *_context->marketData() << _context->orderApi()->getPosition();
    }
}


void TestEnv::sleep(const utility::datetime& time_) const {
    if (_events == 0) {
        return;
    }
    auto now = utility::datetime::utc_now();
    if (not _lastDispatch.actual_time.is_initialized()) {
        LOGWARN("last dispatch time is uninitialised, will not sleep.");
        return;
    }
    auto mktTimeDiff = time_diff(time_, _lastDispatch.mkt_time);

    auto timeSinceLastDispatch = time_diff(now, _lastDispatch.actual_time);

    if (timeSinceLastDispatch < mktTimeDiff /*the amount of time passed, is less than in the market*/) {
        LOGDEBUG(LOG_NVP("TimeNow",now.to_string(utility::datetime::ISO_8601))
                         << LOG_NVP("MktTime",time_.to_string(utility::datetime::ISO_8601)));
        LOGDEBUG(LOG_NVP("LastDispatch",_lastDispatch.actual_time.to_string(utility::datetime::ISO_8601))
                         << LOG_NVP("LastDispatchMktTime",_lastDispatch.mkt_time.to_string(utility::datetime::ISO_8601)));
        LOGDEBUG(LOG_VAR(mktTimeDiff) << LOG_VAR(timeSinceLastDispatch));
        auto sleep_for = mktTimeDiff-timeSinceLastDispatch;
        LOGDEBUG(LOG_VAR(sleep_for));
        std::this_thread::sleep_for(std::chrono::milliseconds(mktTimeDiff-timeSinceLastDispatch));
    }

}
