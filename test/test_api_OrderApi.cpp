//
// Created by rory on 03/07/2021.
//
#include <gtest/gtest.h>
#include <Config.h>
#include <MarketData.h>
#include "api/OrderApi.h"

using namespace io::swagger::client;

TEST(OrderApi, order_newBulk) {

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
    auto orderManager = std::make_shared<api::OrderApi>(apiClient);

    std::string orders = R"([{"clOrdID":"MCST0","orderID":"","orderQty":50,"price":33709.5,"side":"Buy","symbol":"XBTUSD"},{"clOrdID":"MCST1","orderID":"","orderQty":50,"price":33710,"side":"Sell","symbol":"XBTUSD"}])";
    auto newOrders = orderManager->order_newBulk(orders).then(
            [](const std::vector<std::shared_ptr<model::Order>> &orders_) {
                for (auto order : orders_) {
                    std::cout << order->toJson().serialize() << '\n';
                }
            }
    );
    newOrders.get();
}