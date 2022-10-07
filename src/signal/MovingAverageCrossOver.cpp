//
// Created by Rory McStay on 09/08/2021.
//

#include "MovingAverageCrossOver.h"
#include "Utils.h"
#include <exception>
#include <stdexcept>
#include <string>

MovingAverageCrossOver::MovingAverageCrossOver(
            const std::shared_ptr<MarketDataInterface>& marketData_,
            SMA_T::input_type short_,
            SMA_T::input_type long_)
:   Signal(marketData_,
        "moving_average_crossover",
        "short_term_"+std::to_string(short_)
            +",long_term_"+std::to_string(long_)
            +",direction,timestamp"
    )
,   _shortTerm(short_, 10000)
,   _longTerm(long_, 100000)
,   _shortTermVal(0.0)
,   _longTermVal(0.0)
,   _spread(0.0)
,   _direction(SignalDirection::Buy)
,   _is_ready(false)
{
}


void MovingAverageCrossOver::onQuote(
        const std::shared_ptr<model::Quote> &quote_
    ) {
    auto midpoint = (quote_->getAskPrice()+quote_->getBidPrice())/2.0;
    _shortTermVal = _shortTerm(midpoint);
    _longTermVal = _longTerm(midpoint);
    _spread = _shortTermVal - _longTermVal;
    auto new_direction = tradingo_utils::greater_than(_spread, 0.0) ? 
                     SignalDirection::Buy : SignalDirection::Sell;
    if (not _is_ready and _direction != new_direction) {
        // ready on first cross
        _is_ready = true;
    } else if (_is_ready) {
        _direction = new_direction;
    }
}

Signal::Value MovingAverageCrossOver::read() {
    Signal::Value out {_spread, _direction};
    return out;
}

bool MovingAverageCrossOver::isReady() {
    return _is_ready and _shortTerm.is_ready() and _longTerm.is_ready();
}

void MovingAverageCrossOver::init(
        const std::shared_ptr<Config>& config_
    ) {
    Signal::init(config_);
}

std::string MovingAverageCrossOver::read_as_string() {
    std::stringstream out;
    std::string dir; // use a tertiary
    switch(_direction) {
        case (SignalDirection::Buy): { dir = "Buy"; break; }
        case (SignalDirection::Sell): { dir = "Sell"; break; }
        case (SignalDirection::Hold): { dir = "Hold"; break; }
        default: {throw std::runtime_error("Uknonwn signal direction");}
    }
    out << _shortTermVal
        << ',' << _longTermVal
        << ',' << dir
        << ','  << _time.to_string(utility::datetime::ISO_8601);
    return out.str();
}
