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
        auto quote = _marketData->quote();

#if defined(REPLAY_MODE) || !defined(__REPLAY_MODE_GUARD) // toggle REPLAY_MODE in replay live/fast
        // this is test code only!

        if (not _callback) {
            auto mkt_time_past = _timer.interval();
            if (_time.is_initialized()) {
                mkt_time_past = time_diff(quote->getTimestamp(), _time);
            }
            LOGDEBUG("REPLAYE_MODE: Market time past " << LOG_VAR(mkt_time_past) << LOG_VAR(_timer.interval()));
            if (mkt_time_past >= _timer.interval()) {
                LOGDEBUG("REPLAY_MODE: Time to update " << _timer.interval());
                onQuote(quote);
                _time = quote->getTimestamp();
            }
        } else {
            // TODO need to properly implement callback, as we can add extra functionality.
            // such as callback on trade, exec etc...
            LOGDEBUG("REPLAY_MODE: Callback signal update");
            onQuote(quote);
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
    if (_config->get(_name+"-callback") == "true") {
        // if its a callback siganl, always set as callback.
        _callback = true;
        LOGINFO("strategy callback signal initialised." << LOG_VAR(_name));
    } else if (_config->get("override-signal-callback", "false") != "false") {
        // production mode for timer signals.
        _callback = false;
        LOGINFO("Using callback timer for signal."
            << LOG_VAR(_name) << LOG_VAR(evalInterval));
        _timer.start(evalInterval, [this] { update(); });
    } else if (_config->get("override-signal-callback") == "false") { 
        _timer.set_interval(evalInterval);
    }
}

