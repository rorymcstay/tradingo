//
// Created by rory on 31/10/2021.
//

#ifndef TRADINGO_MARGINCALCULATOR_H
#define TRADINGO_MARGINCALCULATOR_H
#include <memory>
#include <model/Position.h>
#include <model/Quote.h>

using namespace io::swagger::client;

class MarginCalculator {

    price_t _indexPrice;
    double _interestRate;
    double _discountFactor;
    int _fundingInterval;
    double _maintenanceMargin;
    double _leverage;
public:
    void operator()(std::shared_ptr<model::Quote> quote_);
    double getUnrealisedPnL(std::shared_ptr<model::Position> position_);
    double getMarkPrice();
    double getMarginAmount(std::shared_ptr<model::Position> position_);

};


#endif //TRADINGO_MARGINCALCULATOR_H
