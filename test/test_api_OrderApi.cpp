#include <gtest/gtest.h>
#include <chrono>

// src
#include "Config.h"
#include "MarketData.h"
#include "Context.h"

// CppRestSwaggerClient
#define _TURN_OFF_PLATFORM_STRING
#include <api/OrderApi.h>
#include <api/PositionApi.h>

#include "fwk/ApiManager.h"
#include "fwk/TestMarketData.h"
#include "fwk/TestEnv.h"

using namespace io::swagger::client;

auto ORDER_PRICE = 47000;
auto ORDER_QTY = 100;
auto SIDE = "Buy";


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
    context->strategy()->allocations()->placeAllocations();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // results in an amend
    context->strategy()->allocations()->addAllocation(ORDER_PRICE, 2*ORDER_QTY);
    context->strategy()->allocations()->placeAllocations();
    // this loops over every price level, need to keep an 'occupied' index to loop over which are indices of the price
    // level vector. also provide a default to cancel all orders rather than passing an always true predicate.
    context->strategy()->allocations()->cancelOrders([](const std::shared_ptr<Allocation>& alloc_) { return true; } /* all */);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(OrderApi, DISABLED_order_newBulk) {

    ApiManager om {};
    auto orders = web::json::value::parse(R"([
        {
            "clOrdID":"MCST0",
            "orderID":"",
            "orderQty":100,
            "price":33709.5,
            "side":"Buy",
            "symbol":"XBTUSD"
        },
        {
            "clOrdID":"MCST1",
            "orderID":"",
            "orderQty":50,
            "price":33710,
            "side":"Sell",
            "symbol":"XBTUSD"
        }
    ])").as_array();
    om.send_orders(orders);
}

TEST(OrderApi, DISABLED_order_newBulk_throws_ApiException) {

    ApiManager om {};

    auto orders = web::json::value::parse(R"([
        {
            "clOrdID":"MCST0",
            "orderID":"",
            "orderQty":100,
            "price":33709.5,
            "side":"Buy",
            "symbol":"XBTUSD"
        },
        {
            "clOrdID":"MCST0",
            "orderID":"",
            "orderQty":50,
            "price":33710,
            "side":"Sell",
            "symbol":"XBTUSD"
        }
    ])").as_array();
    om.send_orders(orders);

}
