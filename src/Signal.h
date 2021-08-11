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


using namespace io::swagger::client;

class Signal {

protected:
    std::shared_ptr<Config> _config;
    std::string _name;

    std::shared_ptr<BatchWriter<std::string>> _batchWriter;




public:
    using Ptr = std::shared_ptr<Signal>;
    using Map = std::unordered_map<std::string, std::shared_ptr<Signal>>;
    using Writer = BatchWriter<std::string>;
    explicit Signal(nullptr_t pVoid) : _config(nullptr){};
    virtual std::string read_as_string() = 0;


    virtual void init(const std::shared_ptr<Config>& config_);;
    void update(std::shared_ptr<Event> md_);
    virtual void onTrade(const std::shared_ptr<model::Trade>& trade_) {};
    virtual void onQuote(const std::shared_ptr<model::Quote>& quote_) {};
    virtual void onExec(const std::shared_ptr<model::Execution>& exec_) {};
    virtual long read() = 0;
    virtual bool isReady() = 0;

    const std::string& name() const { return _name; }


};


#endif //TRADINGO_SIGNAL_H
