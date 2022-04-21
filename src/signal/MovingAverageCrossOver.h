//
// Created by Rory McStay on 09/08/2021.
//

#ifndef MY_PROJECT_MOVINGAVERAGECROSSOVER_H
#define MY_PROJECT_MOVINGAVERAGECROSSOVER_H

#include "ExponentialMovingAverage.h"
#include "Signal.h"

using namespace io::swagger::client;

class MovingAverageCrossOver : public Signal {

    using SMA_T = ExponentialMovingAverage<double, double>;
    //using SMA_T = SimpleMovingAverage<long, long>;

    SMA_T _shortTerm;
    long _shortTermVal;
    long _longTermVal;
    SMA_T _longTerm;

public:
    MovingAverageCrossOver(
            const std::shared_ptr<MarketDataInterface>& marketData_,
            SMA_T::input_type short_,
            SMA_T::input_type long_
            );
    void onQuote(const std::shared_ptr<model::Quote>& quote_) override;
    long read() override;
    bool isReady() override;
    void init(const std::shared_ptr<Config>& config_) override;

    std::string read_as_string() override;

};
#endif //MY_PROJECT_MOVINGAVERAGECROSSOVER_H
