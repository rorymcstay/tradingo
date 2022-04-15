//
// Created by Rory McStay on 05/07/2021.
//

#include "Signal.h"
#include "Config.h"


Signal::Signal(
            const std::shared_ptr<MarketDataInterface>& marketData_,
            const std::string& name_,
            const std::string& header_
)
:   _config(nullptr)
,   _name(name_)
,   _marketData(marketData_)
,   _timer()
,   _time()
,   _callback(false)
,   _header(header_) {

};


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
            LOGDEBUG("REPLAY_MODE: Market time past " << LOG_VAR(mkt_time_past) << LOG_VAR(_timer.interval()));
            if (mkt_time_past >= _timer.interval()) {
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

void Signal::init(const std::shared_ptr<Config> &config_) {
    _config = config_;
    auto storage = config_->get<std::string>("storage", "");
    if (storage.empty())
        storage = "/tmp/";
    auto printer = [](const std::string& it_) { return it_; };
    _batchWriter = std::make_shared<Signal::Writer>(
            /*tableName_=*/_name,
            /*symbol_=*/config_->get<std::string>("symbol"),
            /*storage_=*/storage,
            /*batchSize_=*/100000,
            /*print_=*/printer,
            /*rotate_=*/false,
            /*fileExtension_=*/"csv",
            /*header_=*/_header);
    auto evalInterval = _config->get<int>(_name+"-interval", 1000);
    if (_config->get<bool>(_name+"-callback")) {
        // if its a callback siganl, always set as callback.
        _callback = true;
        LOGINFO("strategy callback signal initialised." << LOG_VAR(_name));
    } else if (not _config->get<bool>("override-signal-callback", false)) {
        // production mode for timer signals.
        _callback = false;
        LOGINFO("Using callback timer for signal."
            << LOG_VAR(_name) << LOG_VAR(evalInterval));
        _timer.start(evalInterval, [this] { update(); });
    } else if (not _config->get<bool>("override-signal-callback")) {
        _timer.set_interval(evalInterval);
    }
}

