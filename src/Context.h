//
// Created by rory on 30/06/2021.
//

#ifndef TRADINGO_CONTEXT_H
#define TRADINGO_CONTEXT_H
#include "MarketData.h"
#include "ApiConfiguration.h"
#include "ApiClient.h"

class Context {

    std::shared_ptr<MarketData> _marketData;
    std::shared_ptr<api::ApiConfiguration> _apiConfig;
    std::shared_ptr<api::ApiClient> _apiClient;
    std::string _apiKey;
public:
    Context(std::shared_ptr<MarketData> md_, std::shared_ptr<api::ApiConfiguration> apiConfig_, std::string apiKey_);

    const std::shared_ptr<MarketData>& marketData() const { return _marketData; }
    const std::shared_ptr<api::ApiConfiguration>& apiConfig() const { return _apiConfig; }
    const std::shared_ptr<api::ApiClient>& apiClient() const { return _apiClient; }

    void authorize(web::http::client::http_client_config& config_ );
};


#endif //TRADINGO_CONTEXT_H
