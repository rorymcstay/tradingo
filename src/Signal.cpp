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
    if (_marketData && _marketData->quote()) {
        LOGDEBUG("Updating Signal " << LOG_VAR(_name));
        // TODO here should check the time in replay mode
        auto quote = _marketData->quote();

#if defined(REPLAY_MODE) || !defined(__REPLAY_MODE_GUARD) // toggle REPLAY_MODE in replay live/fast

        auto mkt_time_past = quote->getTimestamp() - _time;
        LOGDEBUG("Market time past " << LOG_VAR(mkt_time_past) << LOG_VAR(_timer.interval()));
        if (mkt_time_past >= _timer.interval()/1000) {
            LOGDEBUG("Time to update " << _timer.interval());
            onQuote(quote);
            _time = quote->getTimestamp();
        }
#else
        onQuote(quote);
#endif

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
    if (_config->get("callback-signals", "false") == "true") {
        // we use callback instead.
        LOGINFO("Using Callback on signal from TestEnv.");
    } else {
        LOGINFO("Using callback timer for signals");
        _timer.start(evalInterval, std::bind(&Signal::update, this));
    }

}