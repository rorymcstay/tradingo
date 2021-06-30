//
// Created by rory on 30/06/2021.
//

#include "Context.h"
#include "absl/strings/str_join.h"
#include <cpprest/http_client.h>

Context::Context(std::shared_ptr<MarketData> md_, std::shared_ptr<api::ApiConfiguration> apiConfig_, std::string apiKey_)
:   _marketData(std::move(md_))
,   _apiConfig(std::move(apiConfig_))
,   _apiClient(std::make_shared<api::ApiClient>(_apiConfig))
,   _apiKey(std::move(apiKey_)) {

    _apiConfig->setApiKey("api-key", _apiKey);
}


void Context::authorize(web::http::client::http_client_config& config_) {
    _apiClient->getConfiguration()->getHttpConfig();
    std::string signature;
    _apiConfig->setApiKey("api-signature", signature);
}
