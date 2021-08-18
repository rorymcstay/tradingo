//
// Created by Rory McStay on 09/08/2021.
//

#ifndef MY_PROJECT_MOVINGAVERAGECROSSOVER_H
#define MY_PROJECT_MOVINGAVERAGECROSSOVER_H

#include "SimpleMovingAverage.h"
#include "Signal.h"

using namespace io::swagger::client;

class MovingAverageCrossOver : public Signal {

    using SMA_T = SimpleMovingAverage<uint64_t, uint64_t>;

    SMA_T _shortTerm;
    long _shortTermVal;
    long _longTermVal;
    SMA_T _longTerm;
    utility::datetime _time;

public:
    MovingAverageCrossOver(SMA_T::size_t short_, SMA_T::size_t long_);
    void onQuote(const std::shared_ptr<model::Quote>& quote_) override;
    long read();
    bool isReady() override;
    void init(const std::shared_ptr<Config>& config_, std::shared_ptr<MarketDataInterface> md_) override;

    std::string read_as_string() override;

};
#endif //MY_PROJECT_MOVINGAVERAGECROSSOVER_H
