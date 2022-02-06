//
// Created by rory on 31/10/2021.
//
#include "MarginCalculator.h"
#include "Functional.h"
#include <memory>
#include <stdexcept>
#include <utility>

#define _TURN_OFF_PLATFORM_STRING
#include <model/Funding.h>

#include "Utils.h"
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
    return (getMarkPrice()*position_->getCurrentQty()) - position_->getCurrentCost();
}


void MarginCalculator::operator()(const std::shared_ptr<model::Quote>& quote_) {
    _indexPrice = (quote_->getBidPrice() + quote_->getBidPrice())/2.0;
}


double MarginCalculator::getMarkPrice() const {
    return _marketData->instrument()->getMarkPrice();
}


double MarginCalculator::getMarginAmount(const std::shared_ptr<model::Position>& position_) const {
    auto mainMargin = _marketData->instrument()->getMaintMargin();
    return mainMargin * position_->getCurrentQty() * (1/getMarkPrice());
}


double MarginCalculator::getLiquidationPrice(
            double leverage_,
            const std::string& leverageType_,
            price_t fairPrice_,
            qty_t qty_,
            qty_t balance_) const {
    auto maintMargin = _marketData->instrument()->getMaintMargin();
    auto position = _marketData->getPositions().at("XBTUSD");
    auto posValue = position->getMarkValue();

    if (almost_equal(posValue, 0.0)) {
        return 0.0;
    }
    if (leverageType_ == "ISOLATED") {
        auto maint_margin_amount = maintMargin * posValue/leverage_;
        auto liqPricePct = 1 - ((posValue - maint_margin_amount)/posValue);
        return liqPricePct * fairPrice_;
    } else if (leverageType_ == "CROSSED") {
        auto maint_margin_amount = maintMargin * posValue/leverage_ + balance_;
        auto liqPricePct = 1 - ((posValue - maint_margin_amount)/posValue);
        return liqPricePct * fairPrice_;
    }
    else {
        throw std::runtime_error("leverageType_ not recognised " + leverageType_);
    }

}


double MarginCalculator::getOrderCost(double price_, double qty_) const {
    return price_ * qty_;
}


double MarginCalculator::getInitialMargin(double price_, double qty_, double leverage_) const {
    auto value = func::get_cost(price_, qty_, leverage_);
    return 1/leverage_ * value;
}


const std::string& MarginCalculator::leverageType() const {
    return _leverageType;
}


void MarginCalculator::setLeverageType(const std::string& leverageType_) {
    _leverageType = leverageType_;
}
