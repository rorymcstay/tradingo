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
    /// set base variables, setupLogger, order manager
    explicit Context(const std::shared_ptr<Config>& config_);

    factoryMethod_t loadFactoryMethod();

    /// set log level, log to file if configured.
    void setupLogger();

    /// start market data
    void init();

    /// load symbol from library file, download instrument from InstrumentApi, and call strategy::init
    void initStrategy();

    const std::shared_ptr<TMarketData>& marketData() const { return _marketData; }
    const std::shared_ptr<TOrderApi>& orderApi() const { return _orderManager; }
    const std::shared_ptr<api::ApiConfiguration>& apiConfig() const { return _apiConfig; }
    const std::shared_ptr<api::ApiClient>& apiClient() const { return _apiClient; }
    const std::shared_ptr<Strategy<TOrderApi>>& strategy() const { return _strategy; }
    const std::shared_ptr<Config>& config() const { return _config; }

};


template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::init() {
    _instrumentApi = std::make_shared<api::InstrumentApi>(_apiClient);
    _marketData->setInstrumentApi(_instrumentApi);
    _marketData->init();
    _marketData->subscribe();
}

template<typename TMarketData, typename TOrderApi>
typename Context<TMarketData, TOrderApi>::factoryMethod_t
Context<TMarketData, TOrderApi>::loadFactoryMethod() {
    std::string factoryMethodName = _config->get("factoryMethod");
    std::string lib = _config->get("libraryLocation");
    LOGINFO("Loading strategy ... " << LOG_VAR(lib) << LOG_VAR(factoryMethodName));
    _handle = dlopen(lib.c_str(), RTLD_LAZY);
    if (!_handle) {
        LOGERROR( "Cannot open library: " << LOG_VAR(lib) << LOG_VAR(dlerror()));
        std::stringstream message;
        message << "Couldn't load " << LOG_VAR(lib) << LOG_VAR(dlerror());
        throw std::runtime_error(message.str());
    }

    // load the symbol
    factoryMethod_t factoryFunc;
    factoryFunc = (factoryMethod_t)dlsym(_handle, factoryMethodName.c_str());
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        LOGERROR("Cannot load symbol " << LOG_VAR(factoryMethodName) << LOG_VAR(dlsym_error));
        dlclose(_handle);
        std::stringstream msg;
        msg << "Couln't load symbol " << LOG_VAR(factoryMethodName) << LOG_VAR(dlsym_error);
        throw std::runtime_error(msg.str());
    }

    return factoryFunc;

}


template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::Context(const std::shared_ptr<Config>& config_) {

    LOGINFO("Context created. GIT_HASH='" << GIT_HASH << '\'' );

    _config = config_;
    setupLogger();

    _marketData = std::make_shared<TMarketData>(config_);
    _apiConfig = std::make_shared<api::ApiConfiguration>();
    _httpConfig = web::http::client::http_client_config();

    _apiConfig->setBaseUrl(config_->get("baseUrl", "NO HTTP"));
    _apiConfig->setApiKey("api-key", config_->get("apiKey", "NO AUTH"));
    _apiConfig->setApiKey("api-secret", config_->get("apiSecret", "NO AUTH"));
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
    auto logLevel = AixLog::Filter(AixLog::to_severity(_config->get("logLevel", "info")));
    std::vector<std::shared_ptr<AixLog::Sink>> sinks =
    {
        /// Log normal (i.e. non-special) logs to SinkCout
        std::make_shared<AixLog::SinkCout>(logLevel),
                /// Log error and higher severity messages to cerr
                std::make_shared<AixLog::SinkCerr>(AixLog::Severity::error),
    };
    auto logDir =_config->get("logFileLocation", "");
    if (not logDir.empty()) {
        if (not std::filesystem::exists(logDir))
        {
            LOGINFO("Creating new directory " << LOG_VAR(logDir));
            std::filesystem::create_directories(logDir);
        }

        auto logFile = _config->get("logFileLocation") + "/"
                + _config->get("symbol") + "_" + formatTime(std::chrono::system_clock::now())+".log";
        LOGINFO("Logging to " << LOG_VAR(logFile));
        auto sink = std::make_shared<AixLog::SinkFile>(logLevel, logFile);
        sinks.emplace_back(sink);
    }

    AixLog::Log::init(sinks);
}


template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::initStrategy() {

    // TODO: move out to seperate initialiser so tickRecorder can reuse the context.
    Context<TMarketData,TOrderApi>::factoryMethod_t factoryMethod = loadFactoryMethod();
    std::shared_ptr<Strategy<TOrderApi>> strategy = factoryMethod(_marketData,_orderManager);
    _strategy = strategy;


    if (_config->get("httpEnabled", "True") == "True") {
    }
    // block caller thread until above returns.
    // do last
    _strategy->init(_config);


}


#endif //TRADINGO_CONTEXT_H
