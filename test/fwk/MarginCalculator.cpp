//
// Created by rory on 31/10/2021.
//

#include "MarginCalculator.h"

double MarginCalculator::getUnrealisedPnL(std::shared_ptr <model::Position> position_) {
    return (_indexPrice*position_->getCurrentQty()) - position_->getCurrentCost();
}

void MarginCalculator::operator()(std::shared_ptr<model::Quote> quote_) {
    _indexPrice = (quote_->getBidPrice() + quote_->getBidSize())/2.0;
}

double MarginCalculator::getMarkPrice() {
    return _indexPrice;
}

double MarginCalculator::getMarginAmount(std::shared_ptr<model::Position> position_) {
    return _maintenanceMargin * position_->getCurrentQty() * (1/_indexPrice)
}
