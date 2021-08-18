//
// Created by Rory McStay on 05/07/2021.
//

#include "Signal.h"
#include "Config.h"

void Signal::update() {
    /*
    if (_marketData->trade())
        onTrade(_marketData->trade());
    else if (_marketData->exec())
        onExec(_marketData->exec());
    */
    if (_marketData->quote()) {
        onQuote(_marketData->quote());
    } else {

    }
    if (isReady())
        _batchWriter->write(read_as_string());
}

void Signal::init(const std::shared_ptr<Config> &config_, std::shared_ptr<MarketDataInterface> marketData_) {
    _config = config_;
    _marketData = marketData_;
    auto storage = config_->get("storage", "");
    if (storage.empty())
        storage = "/tmp/";
    auto printer = [](const std::string& it_) { return it_; };
    _batchWriter = std::make_shared<BatchWriter<std::string>>(
            "moving_average_crossover",
            config_->get("symbol"),
            storage, 100000, printer);
    auto evalInterval = std::stoi(_config->get(_name+"-interval", "1000"));
    _timer.start(evalInterval, std::bind(&Signal::update, this));

}
