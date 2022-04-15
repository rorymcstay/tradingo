//
// Created by Rory McStay on 18/06/2021.
//

#ifndef TRADINGO_TESTMARKETDATA_H
#define TRADINGO_TESTMARKETDATA_H

#include "TestUtils.h"
#include "Params.h"
#include "Config.h"

#include "MarketData.h"
#include "model/Margin.h"


struct Dispatch {
    utility::datetime mkt_time;
    utility::datetime actual_time;
};

class TestMarketData : public MarketDataInterface {
    std::shared_ptr<Config> _config;
    utility::datetime _time;
    int _events;
    Dispatch _lastDispatch;
    bool _realtime;

    template<typename T>
    void onEvent(const T& event_);
    void sleep(const utility::datetime& time_) const;

public:
    TestMarketData(const std::shared_ptr<Config>& ptr, const std::shared_ptr<InstrumentService>& instSvc_);
    TestMarketData();
    void init();
    void subscribe() {}
    utility::datetime time() const { return _time; }

    void operator << (const std::string& marketDataString);
    void operator << (const std::shared_ptr<model::Quote>& quote_);

    void operator << (const std::shared_ptr<model::Trade>& trade_);
    void operator << (const std::shared_ptr<model::Execution>& exec_);
    void operator << (const std::shared_ptr<model::Position> &pos_);
    void operator << (const std::shared_ptr<model::Order> &order_);
    void operator << (const std::shared_ptr<model::Instrument> &instrument_);
    void operator << (const std::shared_ptr<model::Margin> &margin_);
};


#endif //TRADINGO_TESTMARKETDATA_H
