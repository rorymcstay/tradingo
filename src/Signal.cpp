//
// Created by Rory McStay on 05/07/2021.
//

#include "Signal.h"

Signal::Signal(const std::shared_ptr<Config>& config_)
:   _signalBuffer( std::stoi(config_->get("signalBufferSize", "100")), -1)
,   _dataBuffer(
        std::stoi(config_->get("dataBufferSize", "50")),
        std::vector<double>(std::stoi(config_->get("numCols")), -1.0)) {

}

void Signal::init() {

}
