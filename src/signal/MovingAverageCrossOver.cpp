//
// Created by Rory McStay on 09/08/2021.
//

#include "MovingAverageCrossOver.h"
#include <string>

MovingAverageCrossOver::MovingAverageCrossOver(
            const std::shared_ptr<MarketDataInterface>& marketData_,
            SMA_T::input_type short_,
            SMA_T::input_type long_)
:   Signal(marketData_,
        "moving_average_crossover",
        "short_term_"+std::to_string(short_)
            +",long_term_"+std::to_string(long_)
            +",timestamp"
    )
,   _shortTerm(short_, 100)
,   _longTerm(long_, 100)  {
}


void MovingAverageCrossOver::onQuote(const std::shared_ptr<model::Quote> &quote_) {
    // TODO check if bid or ask has changed.
    _shortTermVal = _shortTerm((quote_->getAskPrice()+quote_->getBidPrice())/2.0);
    _longTermVal = _longTerm((quote_->getAskPrice()+quote_->getBidPrice())/2.0);
}

long MovingAverageCrossOver::read() {
    return _shortTermVal - _longTermVal;
}

bool MovingAverageCrossOver::isReady() {
    return _longTerm.is_ready() && _shortTerm.is_ready();
}

void MovingAverageCrossOver::init(const std::shared_ptr<Config>& config_) {
    Signal::init(config_);
}

std::string MovingAverageCrossOver::read_as_string() {
    std::stringstream out;
    out << _shortTermVal << ',' << _longTermVal << ',' << _time.to_string(utility::datetime::ISO_8601);
    return out.str();
}
