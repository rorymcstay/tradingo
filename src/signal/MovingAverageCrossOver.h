//
// Created by Rory McStay on 09/08/2021.
//

#ifndef MY_PROJECT_MOVINGAVERAGECROSSOVER_H
#define MY_PROJECT_MOVINGAVERAGECROSSOVER_H

#include "SimpleMovingAverage.h"
#include "Signal.h"


class MovingAverageCrossOver : public Signal {

    using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;

    SMA_T _shortTerm;
    long _shortTermVal;
    long _longTermVal;
    SMA_T _longTerm;

public:
    MovingAverageCrossOver(SMA_T::size_t short_, SMA_T::size_t long_);
    void onQuote(const std::shared_ptr<model::Quote>& quote_) override;
    long read();
    bool isReady() override;
    void init(const std::shared_ptr<Config>& config_) override;

};

MovingAverageCrossOver::MovingAverageCrossOver(SMA_T::size_t short_, SMA_T::size_t long_)
: Signal(nullptr),  _shortTerm(short_, 1.0)
, _longTerm(long_, 1.0)  {

    _name = "moving_average_crossover";

}


void MovingAverageCrossOver::onQuote(const std::shared_ptr<model::Quote> &quote_) {

    _shortTermVal = _shortTerm((quote_->getAskPrice()+quote_->getBidPrice())/2.0);
    _longTermVal = _longTerm((quote_->getAskPrice()+quote_->getBidPrice())/2.0);
}

long MovingAverageCrossOver::read() {
    return _shortTermVal - _longTermVal;
}

bool MovingAverageCrossOver::isReady() {
    return false;
}

void MovingAverageCrossOver::init(const std::shared_ptr<Config>& config_) {
    Signal::init(config_);



}


#endif //MY_PROJECT_MOVINGAVERAGECROSSOVER_H
