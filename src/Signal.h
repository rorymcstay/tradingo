//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_SIGNAL_H
#define TRADINGO_SIGNAL_H


#include <memory>

#include <model/Trade.h>
#include <model/Execution.h>
#include <model/Quote.h>

#include "Config.h"
#include "Event.h"
#include "BatchWriter.h"
#include "CallbackTimer.h"
#include "MarketData.h"


using namespace io::swagger::client;


enum class SignalDirection {
    Buy, Sell, Hold
};


class Signal {

protected:


    std::shared_ptr<Config> _config;
    std::string _name;

    std::shared_ptr<BatchWriter<std::string>> _batchWriter;

    CallbackTimer _timer;
    std::shared_ptr<MarketDataInterface> _marketData;
    utility::datetime _time;
    bool _callback;
    std::string _header;

public:
    struct Value {
        double value;
        SignalDirection direction;
    };
    using Ptr = std::shared_ptr<Signal>;
    using Map = std::unordered_map<std::string, std::shared_ptr<Signal>>;
    using Writer = BatchWriter<std::string>;
    Signal(
            const std::shared_ptr<MarketDataInterface>& marketData_,
            const std::string& name_,
            const std::string& header_=""
    );
    virtual std::string read_as_string() = 0;

    virtual void init(const std::shared_ptr<Config>& config_);
    void update();
    virtual void onTrade(const std::shared_ptr<model::Trade>& trade_) {};
    virtual void onQuote(const std::shared_ptr<model::Quote>& quote_) {};
    virtual void onExec(const std::shared_ptr<model::Execution>& exec_) {};
    virtual Value read() = 0;
    virtual bool isReady() = 0;

    const std::string& name() const { return _name; }
    const bool callback() const { return _callback; }


};


#endif //TRADINGO_SIGNAL_H
