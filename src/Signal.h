//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_SIGNAL_H
#define TRADINGO_SIGNAL_H


#include <memory>

#include "Config.h"
#include "Event.h"


class MarketData;

class Signal {

    std::shared_ptr<Config> _config;

    /// buffer of values which have been calculated
    std::vector<double> _signalBuffer;
    /// buffer of rows to be evaluated
    std::vector<std::vector<double>> _dataBuffer;

    explicit Signal(const std::shared_ptr<Config>& config_);

public:
    using Ptr = std::shared_ptr<Signal>;

    virtual void init();
    virtual double evaluate() = 0;
    virtual double update(std::shared_ptr<Event> md_) = 0;
    virtual Signal::Ptr factory(const std::string& file_) = 0;

};


#endif //TRADINGO_SIGNAL_H
