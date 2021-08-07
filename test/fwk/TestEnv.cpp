#define _TURN_OFF_PLATFORM_STRING
#include <Context.h>
#include <BatchWriter.h>
#include "TestEnv.h"


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _config(std::make_shared<Config>(config_))
,   _position(std::make_shared<model::Position>())
{
    _config->set("libraryLocation", "/usr/lib/libtest_trading_strategies.so");
    _config->set("baseUrl", "https://localhost:8888/api/v1");
    _config->set("apiKey", "dummy");
    _config->set("apiSecret", "dummy");
    _config->set("connectionString", "https://localhost:8888/realtime");
    _config->set("clOrdPrefix", "MCST");
    _config->set("httpEnabled", "False");
    _config->set("tickSize", "0.5");
    _config->set("lotSize", "100");
    if (_config->get("logLevel", "").empty())
        _config->set("logLevel", "debug");
    _config->set("cloidSeed", "0");
    _context = std::make_shared<Context<TestMarketData, OrderApi>>(_config);
    _context->init();
    _context->initStrategy();

    _position->setSymbol(_config->get("symbol"));
    _context->orderApi()->setPosition(_position);
    _context->marketData()->addPosition(_position);
    _context->orderApi()->init(_config);
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *_context->marketData() << value_;
        _context->strategy()->evaluate();
        // TODO make TestException class
    } catch (std::exception& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event <<\n\n      " << value_;
    }
}

void TestEnv::operator>>(const std::string &value_) {
    try {
        *_context->orderApi() >> value_;
    } catch (std::runtime_error& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event >>\n\n      " << value_;
    }
}

template<typename T>
std::shared_ptr<T> getEvent(std::ifstream& fileHandle_) {
    std::string str;
    auto quote = std::make_shared<T>();
    if (not std::getline(fileHandle_, str))
        return nullptr;
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
        LOGINFO(AixLog::Color::GREEN << "TestEnv::IN>> Tradable order found: " << AixLog::Color::GREEN << order_->toJson().serialize()
                << AixLog::Color::GREEN << LOG_VAR(trade_->toJson().serialize()) << AixLog::Color::none);
        auto fillQty = std::min(orderQty, tradeQty);
        auto exec = std::make_shared<model::Execution>();
        exec->setSymbol(order_->getSymbol());
        exec->setSide(side);
        exec->setOrderID(order_->getOrderID());
        exec->setAccount(1.0);
        exec->setLastPx(tradePx);
        exec->setLastQty(fillQty);
        exec->setPrice(orderPx);
        exec->setCumQty(order_->getCumQty()+fillQty);
        exec->setLeavesQty(order_->getLeavesQty()-fillQty);

        auto leavesQty = order_->getLeavesQty() - fillQty;
        if (almost_equal(leavesQty, 0.0)) {
            order_->setOrdStatus("Filled");
        } else {
            order_->setOrdStatus("PartiallyFilled");
        }
        auto cumQty = order_->getCumQty() + fillQty;
        order_->setLeavesQty(leavesQty);
        order_->setCumQty(cumQty);
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

    bool stop = false;
    auto quote = getEvent<model::Quote>(quoteFile);
    auto trade = getEvent<model::Trade>(tradeFile);
    bool hasTrades = true;

    std::vector<std::shared_ptr<model::ModelBase>> outBuffer;
    auto batchWriter = BatchWriter("replay", _context->config()->get("symbol"), _context->config()->get("storage"), 5);

    while (not stop) {
        if (!hasTrades or trade->getTimestamp() >= quote->getTimestamp()) {
            // trades are ahead of quotes, send the quote
            auto time = quote->getTimestamp();
            *_context->orderApi() << time; //  >> std::vector
            *_context->marketData() << quote;
            _context->strategy()->evaluate();
            quote = getEvent<model::Quote>(quoteFile);

            // record replay actions to a file.
            *_context->orderApi() >> batchWriter;
            if (not quote) {
                stop = true;
            }
        } else { // send the trade.
            // TODO Check if trade can match on what we have? then send EXECUTION
            for (auto& order : _context->orderApi()->orders()) {
                auto exec = canTrade(order.second, trade);
                if (exec) {
                    // put resultant execution into marketData.
                    _context->orderApi()->addExecToPosition(exec);
                    LOGINFO(AixLog::Color::GREEN << "TestEnv: Sending Execution=" << AixLog::Color::GREEN << exec->toJson().serialize() << AixLog::Color::none);
                    LOGINFO(AixLog::Color::GREEN << "TestEnv: Position is: " << AixLog::Color::GREEN << _position->toJson().serialize() << AixLog::Color::none);
                    *_context->marketData() << exec;
                    *_context->marketData() << order.second;
                    *_context->marketData() << _context->orderApi()->getPosition();

                    if (exec->getOrdStatus() == "Filled") {
                        LOGDEBUG("Filled order");
                    }
                }
            }
            *_context->marketData() << trade;
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
