//
// Created by rory on 31/10/2021.
//

#ifndef TRADINGO_MARGINCALCULATOR_H
#define TRADINGO_MARGINCALCULATOR_H
#include <memory>
#include <string>

#include <model/Position.h>
#include <model/Quote.h>
#include <model/Funding.h>

#include "Allocation.h"
#include "Utils.h"
#include "Config.h"

using namespace io::swagger::client;

class MarginCalculator {

    price_t _indexPrice;
    double _interestRate;
    double _discountFactor;
    int _fundingInterval;
    double _maintenanceMargin;
    double _leverage;
    std::string _leverageType;

public:
    MarginCalculator(const std::shared_ptr<Config>& config_,
        const std::shared_ptr<model::Funding>& funding_);
    void operator()(const std::shared_ptr<model::Quote>& quote_);
    double getUnrealisedPnL(const std::shared_ptr<model::Position>& position_) const;
    double getMarkPrice() const;
    double getMarginAmount(const std::shared_ptr<model::Position>& position_) const;
    double getLiquidationPrice(const std::shared_ptr<model::Position>& position_, double balance_) const;

};


#endif //TRADINGO_MARGINCALCULATOR_H