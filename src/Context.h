//
// Created by rory on 30/06/2021.
//

#ifndef TRADINGO_CONTEXT_H
#define TRADINGO_CONTEXT_H


#include <iostream>
#include <dlfcn.h>
#include <chrono>
#include <filesystem>

#include <api/OrderApi.h>

#include "MarketData.h"
#include "ApiConfiguration.h"
#include "ApiClient.h"
#include "Strategy.h"
#include "aixlog.hpp"
#include "Utils.h"
#include "api/InstrumentApi.h"
#include "InstrumentService.h"


template<typename TMarketData,
    typename TOrderApi,
    typename TPositionApi>
class Context {

    std::shared_ptr<TMarketData> _marketData;
    std::shared_ptr<api::ApiConfiguration> _apiConfig;
    std::shared_ptr<api::ApiClient> _apiClient;
    web::http::client::http_client_config _httpConfig;
    std::shared_ptr<TOrderApi> _orderManager;
    std::shared_ptr<Strategy<TOrderApi>> _strategy;
    // TODO: Factor out instrument service into templated type, and mock InstrumentApi in test/fwk
    std::shared_ptr<InstrumentService> _instrumentService;
    std::shared_ptr<TPositionApi> _positionApi;
    void* _handle{};

    std::shared_ptr<Config> _config;
    typedef std::shared_ptr<Strategy<TOrderApi>> (*factoryMethod_t)(
            std::shared_ptr<TMarketData>,std::shared_ptr<TOrderApi>, std::shared_ptr<InstrumentService>) ;

public:

    ~Context();
    /// set base variables, setupLogger, order manager
    explicit Context(std::shared_ptr<Config> config_);
    /// load 'factoryMethod' from 'libraryLocation', specified in config.
    factoryMethod_t loadFactoryMethod();
    /// set log level, log to file if configured.
    void setupLogger();
    /// start market data
    void init();
    /// load symbol from library file, download instrument from InstrumentApi, and call strategy::init
    void initStrategy();

    /// accessor functions
    const std::shared_ptr<TMarketData>& marketData() const { return _marketData; }
    const std::shared_ptr<TOrderApi>& orderApi() const { return _orderManager; }
    const std::shared_ptr<api::ApiConfiguration>& apiConfig() const { return _apiConfig; }
    const std::shared_ptr<api::ApiClient>& apiClient() const { return _apiClient; }
    const std::shared_ptr<Strategy<TOrderApi>>& strategy() const { return _strategy; }
    const std::shared_ptr<Config>& config() const { return _config; }
    const std::shared_ptr<InstrumentService>& instrumentService() const { return _instrumentService; }
    const std::shared_ptr<TPositionApi>& positionApi() const { return _positionApi; }

};


template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::init() {
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
Context<TMarketData, TOrderApi>::Context(std::shared_ptr<Config> config_)
:   _config(std::move(config_))
,   _apiConfig(std::make_shared<api::ApiConfiguration>())
,   _httpConfig(web::http::client::http_client_config())

{

    _apiConfig->setBaseUrl(_config->get("baseUrl", "NO HTTP"));
    _apiConfig->setApiKey("api-key", _config->get("apiKey", "NO AUTH"));
    _apiConfig->setApiKey("api-secret", _config->get("apiSecret", "NO AUTH"));
    _apiConfig->setHttpConfig(_httpConfig);
    _apiClient = std::make_shared<api::ApiClient>(_apiConfig);
    _instrumentService = std::make_shared<InstrumentService>(_apiClient, _config);
    _marketData = std::make_shared<TMarketData>(_config, _instrumentService);
    _orderManager = std::make_shared<TOrderApi>(_apiClient);
    setupLogger();
    LOGINFO("Context created. Version Info: GIT_BRANCH='" << GIT_BRANCH << "' GIT_TAG='" << GIT_TAG
                    << "' GIT_REV='" << GIT_REV << "'");



}

template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::~Context() {
    dlclose(_handle);
}


template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::setupLogger() {
    auto logLevel = AixLog::Filter(AixLog::to_severity(_config->get("logLevel", "info")));
    std::vector<std::shared_ptr<AixLog::Sink>> sinks = {
        std::make_shared<AixLog::SinkCout>(logLevel),
        std::make_shared<AixLog::SinkCerr>(AixLog::Severity::error),
    };
    auto logDir =_config->get("logFileLocation", "");
    if (not logDir.empty()) {
        if (not std::filesystem::exists(logDir)) {
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

    Context<TMarketData,TOrderApi>::factoryMethod_t factoryMethod = loadFactoryMethod();
    std::shared_ptr<Strategy<TOrderApi>> strategy = factoryMethod(_marketData,_orderManager, _instrumentService);
    _strategy = strategy;

    _strategy->init(_config);
    _marketData->setCallback([this]() { _strategy->evaluate(); });

}


#endif //TRADINGO_CONTEXT_H
