#include <gtest/gtest.h>
#include <chrono>

// src
#include "Config.h"
#include "MarketData.h"
#include "Context.h"
#include "fwk/TestMarketData.h"

// CppRestSwaggerClient
#include "api/OrderApi.h"
#include "api/PositionApi.h"

#include "fwk/TestEnv.h"

using namespace io::swagger::client;

auto ORDER_PRICE = 47000;
auto ORDER_QTY = 100;
auto SIDE = "Buy";

struct OrderManager {
    std::shared_ptr<api::OrderApi> orderApi;
    long oidSeed;
    OrderManager()
    :   orderApi(nullptr)
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

TEST(TestStrategyInterface, DISABLED_smoke_test) {
    auto config = std::make_shared<Config>();
    config->set("apiKey", "-rqipjFxM43WSRKdC8keq83K");
    config->set("apiSecret", "uaCYIiwpwpXNKuVGCBPWE3ThzvyhOzKs6F9mWFzc9LueG3yd");
    config->set("symbol", "XBTUSD");
    config->set("baseUrl", "https://testnet.bitmex.com/api/v1/");
    config->set("connectionString", "wss://testnet.bitmex.com");
    INSERT_DEFAULT_ARGS(config);
    auto context = std::make_shared<Context<TestMarketData, api::OrderApi, api::PositionApi>>(config);
    context->initStrategy();
    context->strategy()->allocations()->addAllocation(ORDER_PRICE, ORDER_QTY);
    context->strategy()->allocations()->addAllocation(ORDER_PRICE, ORDER_QTY);
    context->strategy()->placeAllocations();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // results in an amend
    context->strategy()->allocations()->addAllocation(ORDER_PRICE, 2*ORDER_QTY);
    context->strategy()->placeAllocations();
    // this loops over every price level, need to keep an 'occupied' index to loop over which are indices of the price
    // level vector. also provide a default to cancel all orders rather than passing an always true predicate.
    context->strategy()->allocations()->cancelOrders([](const std::shared_ptr<Allocation>& alloc_) { return true; } /* all */);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(OrderApi, DISABLED_order_newBulk) {

    OrderManager om {};

    std::string orders = R"([{"clOrdID":"MCST0","orderID":"","orderQty":100,"price":33709.5,"side":"Buy","symbol":"XBTUSD"},{"clOrdID":"MCST1","orderID":"","orderQty":50,"price":33710,"side":"Sell","symbol":"XBTUSD"}])";
    auto newOrders = om.orderApi->order_newBulk(orders).then(
            [](const std::vector<std::shared_ptr<model::Order>> &orders_) {
                for (const auto& order : orders_) {
                    std::cout << order->toJson().serialize() << '\n';
                }
            }
    );
    newOrders.get();
}

TEST(OrderApi, order_newBulk_throws_ApiException) {

    OrderManager om {};

    model::Order order1 {};
    order1.setOrderQty(100);
    order1.setPrice(33370.5);
    order1.setSide("Buy");
    order1.setClOrdID(std::to_string(om.oidSeed));
    order1.setSymbol("XBTUSD");
    model::Order order2 {};

    // create second order with duplicate ClOrdID
    order2.setOrderQty(100);
    order2.setPrice(33370.5);
    order2.setSide("Buy");
    order2.setClOrdID(std::to_string(om.oidSeed)); // duplicate cloordid
    order2.setSymbol("XBTUSD");

    std::error_code res;
    try {
        auto newOrders = om.orderApi->order_newBulk(om.to_string({order1, order2})).then(
            [](const std::vector<std::shared_ptr<model::Order>> &orders_) {
                for (const auto& order : orders_) {
                    std::cout << order->toJson().serialize() << '\n';
                }
            }
        );
        newOrders.get();
    } catch (api::ApiException& ex_ ) {
        res = ex_.error_code();
        LOGINFO("ApiException: " << ex_.getContent()->rdbuf());
    } catch (web::http::http_exception ex_) {
        LOGINFO("Httpexception!");
    }
    LOGINFO(LOG_VAR(res));
}
