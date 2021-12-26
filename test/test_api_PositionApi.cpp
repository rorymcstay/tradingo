#include <gtest/gtest.h>

#include "fwk/ApiManager.h"
#include <cpprest/json.h>

TEST(TestPositionApi, test_formatting_params) {
    web::json::value body_data = web::json::value::object();
    //body_data["leverage"] = web::json::value::parse("10.0000");
    body_data["symbol"] = web::json::value::parse(R"({"symbol": "XBTUSD"})");
}


TEST(TestPositionApi, test_update_leverage) {

    ApiManager apiMgr{};
    apiMgr.positionApi->position_updateLeverage("XBTUSD", 10.0);

}


TEST(TestPositionApi, DISABLED_test_parse_invalid_json) {
    
    std::cout << web::json::value::parse("10.0000").serialize() << '\n';

}
