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

TestMarketData::TestMarketData(const std::shared_ptr<Config> &ptr) {

}
