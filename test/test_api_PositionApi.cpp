#include <gtest/gtest.h>

#include "fwk/ApiManager.h"
#include <cpprest/json.h>

TEST(TestPositionApi, test_update_leverage) {

    ApiManager apiMgr{};
    apiMgr.positionApi->position_updateLeverage("XBTUSD", 10.0);

}


TEST(TestPositionApi, parse_json_float) {
    
    std::cout << web::json::value::parse("10.0000").serialize() << '\n';

}
