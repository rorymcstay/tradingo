//
// Created by Rory McStay on 05/07/2021.
//

#include "Signal.h"
#include "Config.h"

void Signal::update(std::shared_ptr<Event> md_) {
    if (md_->getTrade())
        onTrade(md_->getTrade());
    else if (md_->getExec())
        onExec(md_->getExec());
    else if (md_->getQuote())
        onQuote(md_->getQuote());
    else {
        return;
    }
    if (isReady())
        _batchWriter->write(read_as_string());
}

void Signal::init(const std::shared_ptr<Config> &config_) {
    _config = config_;
    auto storage = config_->get("storage", "");
    if (storage.empty())
        storage = "/tmp/";
    auto printer = [](const std::string& it_) { return it_; };
    _batchWriter = std::make_shared<BatchWriter<std::string>>(
            "moving_average_crossover",
            config_->get("symbol"),
            storage, 1000, printer);
}
