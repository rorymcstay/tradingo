//
// Created by Rory McStay on 18/06/2021.
//

#ifndef TRADINGO_TESTMARKETDATA_H
#define TRADINGO_TESTMARKETDATA_H

#include "TestUtils.h"
#include "Params.h"
#include "Config.h"

#include "MarketData.h"

class TestMarketData : public MarketDataInterface {
    std::shared_ptr<Config> _config;
    utility::datetime _time;

public:
    explicit TestMarketData(const std::shared_ptr<Config>& ptr);
    void init();
    void subscribe() {}
    utility::datetime time() const { return _time; }

    void addPosition(const std::shared_ptr<model::Position>& position_) {
        _positions[position_->getSymbol()] = position_;
    }

    void operator << (const std::string& marketDataString);
    void operator << (const std::shared_ptr<model::Quote>& quote_);

    void operator << (const std::shared_ptr<model::Trade>& trade_);
    void operator << (const std::shared_ptr<model::Execution>& exec_);
    void operator << (const std::shared_ptr<model::Position> &pos_);
    void operator << (const std::shared_ptr<model::Order> &order_);
};


#endif //TRADINGO_TESTMARKETDATA_H
