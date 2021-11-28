#ifndef TRADINGO_APIMANAGER_H
#define TRADINGO_APIMANAGER_H
#include <chrono>

// src

#include "Config.h"
#include "MarketData.h"
#include "Context.h"
#include "fwk/TestMarketData.h"

// CppRestSwaggerClient
#define _TURN_OFF_PLATFORM_STRING
#include <api/OrderApi.h>
#include <api/PositionApi.h>

struct ApiManager {
    std::shared_ptr<api::OrderApi> orderApi;
    std::shared_ptr<api::PositionApi> positionApi;
    long oidSeed;
    ApiManager()
    :   orderApi(nullptr)
    ,   positionApi(nullptr)
    ,   oidSeed(std::chrono::system_clock::now().time_since_epoch().count()){

        auto config = std::make_shared<Config>();
        config->set("apiKey", "-rqipjFxM43WSRKdC8keq83K");
        config->set("apiSecret", "uaCYIiwpwpXNKuVGCBPWE3ThzvyhOzKs6F9mWFzc9LueG3yd");
        config->set("symbol", "XBTUSD");
        config->set("baseUrl", "https://testnet.bitmex.com/api/v1/");
        config->set("connectionString", "wss://testnet.bitmex.com");
        std::string base_url = config->get("baseUrl");
        auto apiConfig = std::make_shared<api::ApiConfiguration>();
        auto httpConfig = web::http::client::http_client_config();
        apiConfig->setHttpConfig(httpConfig);
        apiConfig->setBaseUrl(base_url);
        apiConfig->setApiKey("api-key", config->get("apiKey"));
        apiConfig->setApiKey("api-secret", config->get("apiSecret"));
        auto apiClient = std::make_shared<api::ApiClient>(apiConfig);
        orderApi = std::make_shared<api::OrderApi>(apiClient);
        positionApi = std::make_shared<api::PositionApi>(apiClient);
    }

    std::string to_string(std::initializer_list<model::Order> orders_) {
        auto lis = web::json::value::array();
        int count = 0;
        for (auto& order : orders_) {
            lis[count++] = order.toJson();
        }
        return lis.serialize();
    }

};

#endif
