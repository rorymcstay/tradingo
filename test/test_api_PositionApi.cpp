#include <gtest/gtest.h>
#include "fwk/ApiManager.h"

TEST(TestPositionApi, test_formatting_params) {
    web::json::value body_data = web::json::value::object();
    //body_data["leverage"] = web::json::value::parse("10.0000");
    body_data["symbol"] = web::json::value::parse(R"({"symbol": "XBTUSD"})");
}

TEST(TestPositionApi, test_update_leverage) {

    ApiManager apiMgr{};
    apiMgr.positionApi->position_updateLeverage("XBTUSD", 10.0);

}
