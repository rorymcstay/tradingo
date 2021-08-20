//
// Created by Rory McStay on 09/08/2021.
//

#include "MovingAverageCrossOver.h"

MovingAverageCrossOver::MovingAverageCrossOver(SMA_T::size_t short_, SMA_T::size_t long_)
        : Signal(),  _shortTerm(short_, 1.0)
        , _longTerm(long_, 1.0)  {
    _name = "moving_average_crossover";
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

void MovingAverageCrossOver::init(const std::shared_ptr<Config>& config_, std::shared_ptr<MarketDataInterface> md_) {
    Signal::init(config_, md_);
}

std::string MovingAverageCrossOver::read_as_string() {
    std::stringstream out;
    out << _shortTermVal << ',' << _longTermVal << ',' << _time.to_string(utility::datetime::ISO_8601);
    return out.str();
}
