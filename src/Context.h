//
// Created by rory on 30/06/2021.
//

#ifndef TRADINGO_CONTEXT_H
#define TRADINGO_CONTEXT_H


#include <iostream>
#include <dlfcn.h>

#include <api/OrderApi.h>
#include "MarketData.h"
#include "ApiConfiguration.h"
#include "ApiClient.h"
#include "Strategy.h"

template<typename TMarketData, typename TOrderApi>
class Context {

    std::shared_ptr<TMarketData> _marketData;
    std::shared_ptr<api::ApiConfiguration> _apiConfig;
    std::shared_ptr<api::ApiClient> _apiClient;
    web::http::client::http_client_config _httpConfig;
    std::shared_ptr<TOrderApi> _orderManager;
    std::shared_ptr<Strategy<api::OrderApi>> _strategy;
    void* _handle;

private:
    std::shared_ptr<Config> _config;
    typedef std::shared_ptr<Strategy<TOrderApi>> (*factoryMethod_t)(
            std::shared_ptr<TMarketData>,std::shared_ptr<TOrderApi>) ;

public:
    Context(std::shared_ptr<TMarketData> md_, std::shared_ptr<api::ApiConfiguration> apiConfig_);

    ~Context();
    explicit Context(std::shared_ptr<Config> config_);

    factoryMethod_t loadFactoryMethod();

    void init();

    const std::shared_ptr<TMarketData>& marketData() const { return _marketData; }
    const std::shared_ptr<api::ApiConfiguration>& apiConfig() const { return _apiConfig; }
    const std::shared_ptr<api::ApiClient>& apiClient() const { return _apiClient; }
    const std::shared_ptr<Strategy<TOrderApi>>& strategy() const { return _strategy; }

};


template<typename TMd, typename TOrdApi>
Context<TMd, TOrdApi>::Context(std::shared_ptr<TMd> md_, std::shared_ptr<api::ApiConfiguration> apiConfig_)
        :   _marketData(std::move(md_))
        ,   _apiConfig(std::move(apiConfig_))
        ,   _apiClient(std::make_shared<api::ApiClient>(_apiConfig)) {

}

template<typename TMarketData, typename TOrderApi>
void Context<TMarketData, TOrderApi>::init() {
    auto factoryMethod = loadFactoryMethod();

    _strategy = factoryMethod(_marketData,_orderManager);
    _marketData->init();
    _marketData->subscribe();
    //_marketData->initSignals(_config);

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
        INFO("Cannot load symbol " <<LOG_VAR(factoryMethodName) << dlsym_error);
        dlclose(_handle);
        std::stringstream msg;
        msg << "Couln't load symbol " << LOG_VAR(factoryMethodName);
        throw std::runtime_error(msg.str());
    }
    // create an instance of the class

    return factoryFunc;

}

template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::Context(std::shared_ptr<Config> config_) {

    _config = std::move(config_);
    std::string lib = _config->get("libraryLocation");
    _handle = dlopen(lib.c_str(), RTLD_LAZY);
    if (!_handle) {
        ERROR( "Cannot open library: " << LOG_VAR(lib) << LOG_VAR(dlerror()));
        std::stringstream message;
        message << "Couldn't load " << LOG_VAR(lib) << LOG_VAR(dlerror());
        throw std::runtime_error(message.str());
    }

    _marketData = std::make_shared<TMarketData>(config_);
    _apiConfig = std::make_shared<api::ApiConfiguration>();
    _httpConfig = web::http::client::http_client_config();
    _config = std::move(config_);

    _apiConfig->setBaseUrl(config_->get("baseUrl"));
    _apiConfig->setApiKey("api-key", config_->get("apiKey"));
    _apiConfig->setApiKey("api-secret", config_->get("apiSecret"));
    _apiConfig->setHttpConfig(_httpConfig);

    _apiClient = std::make_shared<api::ApiClient>(_apiConfig);
    _orderManager = std::make_shared<TOrderApi>(_apiClient);
}

template<typename TMarketData, typename TOrderApi>
Context<TMarketData, TOrderApi>::~Context() {
    dlclose(_handle);
}


#endif //TRADINGO_CONTEXT_H
