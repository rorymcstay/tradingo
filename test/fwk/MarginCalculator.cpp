//
// Created by rory on 31/10/2021.
//
#include "MarginCalculator.h"
#include <memory>
#include <utility>

#define _TURN_OFF_PLATFORM_STRING
#include <model/Funding.h>

#include "Config.h"

MarginCalculator::MarginCalculator(
    const std::shared_ptr<Config>& config_,
    std::shared_ptr<TestMarketData>  marketData_)
:   _indexPrice()
,   _interestRate()
,   _leverageType(config_->get("leverageType", "ISOLATED"))
,   _leverage(std::atof(config_->get("leverage", "10").c_str()))
,   _marketData(std::move(marketData_))
{

}

double MarginCalculator::getUnrealisedPnL(const std::shared_ptr <model::Position>& position_) const {
    return (_indexPrice*position_->getCurrentQty()) - position_->getCurrentCost();
}

void MarginCalculator::operator()(const std::shared_ptr<model::Quote>& quote_) {
    _indexPrice = (quote_->getBidPrice() + quote_->getBidPrice())/2.0;
}

double MarginCalculator::getMarkPrice() const {
    return _marketData->instrument()->getFairPrice();
}

double MarginCalculator::getMarginAmount(const std::shared_ptr<model::Position>& position_) const {
    auto mainMargin = _marketData->instrument()->getMaintMargin();
    return mainMargin * position_->getCurrentQty() * (1/getMarkPrice());
}

double MarginCalculator::getLiquidationPrice(const std::shared_ptr<model::Position>& position_, double balance) const {
    auto symbol = position_->getSymbol();
    auto size = position_->getCurrentQty();
    auto entryPrice = position_->getAvgEntryPrice();
    auto leverage = position_->getLeverage();
    // TODO Calculate liquidation price given margin
    return _indexPrice;
}
