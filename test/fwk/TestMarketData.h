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
    void operator << (const std::string& marketDataString);
};


#endif //TRADINGO_TESTMARKETDATA_H
