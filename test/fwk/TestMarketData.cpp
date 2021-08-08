//
// Created by Rory McStay on 18/06/2021.
//

#include "TestMarketData.h"

void TestMarketData::operator<<(const std::string &marketDataString) {

    auto params = Params(marketDataString);

    auto json = params.asJson();
    auto type = getEventTypeFromString(marketDataString);

    if (type == "EXECUTION") {
        auto execution = fromJson<model::Execution>(json);
        std::vector<decltype(execution)> execs =  {execution};
        handleExecutions(execs, "insert");
    } else if (type == "QUOTE") {
        auto quote = fromJson<model::Quote>(json);
        std::vector<decltype(quote)> qts = {quote};
        handleQuotes(qts, "insert");
    } else if (type == "TRADE") {
        auto trade = fromJson<model::Trade>(json);
        std::vector<decltype(trade)> trades = {trade};
        handleTrades(trades, "insert");
    } else if (type == "POSITION") {
        auto position = fromJson<model::Position>(json);
        std::vector<decltype(position)> positions = {position};
        std::string action;
        if (_positions.contains(getPositionKey(position))) {
            action = "update";
        } else {
            action = "insert";
        }
        handlePositions(positions, action);
    } else {
        throw std::runtime_error("Must specify update type, one off POSITION, TRADE, QUOTE, EXECUTION");
    }
}

TestMarketData::TestMarketData(const std::shared_ptr<Config>& ptr)
: MarketDataInterface(ptr)
, _config(ptr){

}

void TestMarketData::init() {
    auto tickSize = std::atof(_config->get("tickSize", "0.5").c_str());
    auto referencePrice = std::atof(_config->get("referencePrice").c_str());
    auto lotSize = std::atof(_config->get("lotSize", "100").c_str());
    _instrument = std::make_shared<model::Instrument>();
    _instrument->setPrevPrice24h(referencePrice);
    _instrument->setTickSize(tickSize);
    _instrument->setLotSize(lotSize);
}

void TestMarketData::operator<<(const std::shared_ptr<model::Quote> &quote_) {
    std::vector<std::shared_ptr<model::Quote>> quotes = {quote_};
    MarketDataInterface::handleQuotes(quotes, "INSERT");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Trade> &trade_) {
    std::vector<std::shared_ptr<model::Trade>> trades = {trade_};
    MarketDataInterface::handleTrades(trades, "INSERT");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Execution> &exec_) {
    std::vector<std::shared_ptr<model::Execution>> execs = {exec_};
    MarketDataInterface::handleExecutions(execs, "INSERT");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Position> &pos_) {
    std::vector<std::shared_ptr<model::Position>> positions = {pos_};
    MarketDataInterface::handlePositions(positions, "INSERT");
}

void TestMarketData::operator<<(const std::shared_ptr<model::Order> &order_) {
    std::string action = "insert";
    if (order_->getOrdStatus() == "Canceled") {
        action = "delete";
    } else if (order_->getOrdStatus() == "Filled") {
        action = "delete";
    } else if (order_->getOrdStatus() == "Rejected") {
        action = "delete";
    }
    std::vector<std::shared_ptr<model::Order>> orders = {order_};
    MarketDataInterface::handleOrders(orders, action);
}
