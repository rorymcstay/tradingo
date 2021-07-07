//
// Created by Rory McStay on 18/06/2021.
//

#ifndef TRADINGO_TESTMARKETDATA_H
#define TRADINGO_TESTMARKETDATA_H

#include "TestUtils.h"
#include "Params.h"

#include "MarketData.h"

class TestMarketData : public MarketDataInterface {
public:
    explicit TestMarketData(const std::shared_ptr<Config>& ptr);
    void init() {}
    void subscribe() {}
    void operator << (const std::string& marketDataString);
    void operator << (const std::shared_ptr<model::Quote>& quote_) {
        std::vector<std::shared_ptr<model::Quote>> quotes = {quote_};
        MarketDataInterface::handleQuotes(quotes, "INSERT");
    }

    void operator << (const std::shared_ptr<model::Trade>& trade_) {
        std::vector<std::shared_ptr<model::Trade>> trades = {trade_};
        MarketDataInterface::handleTrades(trades, "INSERT");
    }
    void operator << (const std::shared_ptr<model::Execution>& exec_) {
        std::vector<std::shared_ptr<model::Execution>> execs = {exec_};
        MarketDataInterface::handleExecutions(execs, "INSERT");
    }
};


#endif //TRADINGO_TESTMARKETDATA_H
