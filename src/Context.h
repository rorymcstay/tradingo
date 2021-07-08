//
// Created by rory on 30/06/2021.
//

#ifndef TRADINGO_CONTEXT_H
#define TRADINGO_CONTEXT_H


#include <iostream>
#include <dlfcn.h>
#include <chrono>

#include <api/OrderApi.h>
#include <filesystem>
#include "MarketData.h"
#include "ApiConfiguration.h"
#include "ApiClient.h"
#include "Strategy.h"
#include "aixlog.hpp"
#include "Utils.h"
#include "api/InstrumentApi.h"

template<typename TMarketData, typename TOrderApi>
class Context {

    std::shared_ptr<TMarketData> _marketData;
    std::shared_ptr<api::ApiConfiguration> _apiConfig;
    std::shared_ptr<api::ApiClient> _apiClient;
    web::http::client::http_client_config _httpConfig;
    std::shared_ptr<TOrderApi> _orderManager;
    std::shared_ptr<Strategy<TOrderApi>> _strategy;
    std::shared_ptr<api::InstrumentApi> _instrumentApi;
    void* _handle;

private:
    std::shared_ptr<Config> _config;
    typedef std::shared_ptr<Strategy<TOrderApi>> (*factoryMethod_t)(
            std::shared_ptr<TMarketData>,std::shared_ptr<TOrderApi>) ;

public:

    ~Context();
    explicit Context(const std::shared_ptr<Config>& config_);

    factoryMethod_t loadFactoryMethod();

    void setupLogger();

    void init();

    const std::shared_ptr<TMarketData>& marketData() const { return _marketData; }
    const std::shared_ptr<TOrderApi>& orderApi() const { return _orderManager; }
    const std::shared_ptr<api::ApiConfiguration>& apiConfig() const { return _apiConfig; }
    const std::shared_ptr<api::ApiClient>& apiClient() const { return _apiClient; }
    const std::shared_ptr<Strategy<TOrderApi>>& strategy() const { return _strategy; }

};


template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::init() {

    Context<TMarketData,TOrderApi>::factoryMethod_t factoryMethod = loadFactoryMethod();
    std::shared_ptr<Strategy<TOrderApi>> strategy = factoryMethod(_marketData,_orderManager);
    _strategy = strategy;
    _instrumentApi = std::make_shared<api::InstrumentApi>(_apiClient);

#define SEVENNULL boost::none,boost::none,boost::none,boost::none,boost::none,boost::none,boost::none

    _instrumentApi->instrument_get(_config->get("symbol"), SEVENNULL).then(
        [this] (pplx::task<std::vector<std::shared_ptr<model::Instrument>>> instr_) {
            LOGINFO("Instrument: " << instr_.get()[0]->toJson().serialize());
            _strategy->setInstrument(instr_.get()[0]);
        });
    // do last

    _marketData->init();
    _marketData->subscribe();
    _strategy->init(_config);

}

template<typename TMarketData, typename TOrderApi>
typename Context<TMarketData, TOrderApi>::factoryMethod_t
Context<TMarketData, TOrderApi>::loadFactoryMethod() {
    std::string factoryMethodName = _config->get("factoryMethod");
    // load the symbols
    factoryMethod_t factoryFunc;
    factoryFunc = (factoryMethod_t)dlsym(_handle, factoryMethodName.c_str());
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        LOGINFO("Cannot load symbol " << LOG_VAR(factoryMethodName) << dlsym_error);
        dlclose(_handle);
        std::stringstream msg;
        msg << "Couln't load symbol " << LOG_VAR(factoryMethodName);
        throw std::runtime_error(msg.str());
    }
    // create an instance of the class

    return factoryFunc;

}

template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::Context(const std::shared_ptr<Config>& config_) {

    _config = config_;
    std::string lib = _config->get("libraryLocation");

    setupLogger();

    _handle = dlopen(lib.c_str(), RTLD_LAZY);
    if (!_handle) {
        LOGERROR( "Cannot open library: " << LOG_VAR(lib) << LOG_VAR(dlerror()));
        std::stringstream message;
        message << "Couldn't load " << LOG_VAR(lib) << LOG_VAR(dlerror());
        throw std::runtime_error(message.str());
    }

    _marketData = std::make_shared<TMarketData>(config_);
    _apiConfig = std::make_shared<api::ApiConfiguration>();
    _httpConfig = web::http::client::http_client_config();

    _apiConfig->setBaseUrl(config_->get("baseUrl"));
    _apiConfig->setApiKey("api-key", config_->get("apiKey"));
    _apiConfig->setApiKey("api-secret", config_->get("apiSecret"));
    _apiConfig->setHttpConfig(_httpConfig);
    _config = config_;

    _apiClient = std::make_shared<api::ApiClient>(_apiConfig);
    _orderManager = std::make_shared<TOrderApi>(_apiClient);
}

template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::~Context() {
    dlclose(_handle);
}

template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::setupLogger() {

    std::vector<std::shared_ptr<AixLog::Sink>> sinks =
    {
        /// Log normal (i.e. non-special) logs to SinkCout
        std::make_shared<AixLog::SinkCout>(AixLog::Severity::trace),
                /// Log error and higher severity messages to cerr
                std::make_shared<AixLog::SinkCerr>(AixLog::Severity::error),
                /// Log special logs to native log (Syslog on Linux, Android Log on Android, EventLog on Windows, Unified logging on Apple)

                /*make_shared<AixLog::SinkCallback>(AixLog::Severity::trace,
                    [](const AixLog::Metadata& metadata, const std::string& message) {
                      std::cout << "Callback:\n\tmsg:   " << message << "\n\ttag:   " << metadata.tag.text << "\n\tsever: " << AixLog::Log::to_string(metadata.severity) << " (" << (int)metadata.severity << ")\n\ttype:  " << (metadata.type == AixLog::Type::normal?"normal":"special") << "\n";
                      if (metadata.timestamp)
                          std::cout << "\ttime:  " << metadata.timestamp.to_string() << "\n";
                      if (metadata.function)
                          std::cout << "\tfunc:  " << metadata.function.name << "\n\tline:  " << metadata.function.line << "\n\tfile:  " << metadata.function.file << "\n";
                    }
                )*/
    };
    auto logDir =_config->get("logFileLocation", "");
    if (logDir.empty()) {
        if (not std::filesystem::exists(logDir))
        {
            LOGINFO("Creating new directory " << LOG_VAR(logDir));
            std::filesystem::create_directories(logDir);
        }

        auto logFile = _config->get(
                _config->get("logFileLocation") + "/"
                + _config->get("symbol") + "_" + formatTime(std::chrono::system_clock::now())+".log");
        auto sink = std::make_shared<AixLog::SinkFile>(AixLog::Severity::info, logFile);
        sinks.emplace_back(sink);
    }

    AixLog::Log::init(sinks);
}


#endif //TRADINGO_CONTEXT_H
