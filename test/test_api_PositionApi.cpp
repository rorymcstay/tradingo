#include <gtest/gtest.h>
#include "fwk/ApiManager.h"

TEST(TestPositionApi, test_update_leverage) {

    ApiManager apiMgr{};
    apiMgr.positionApi->position_updateLeverage("XBTUSD", 10.0);

}
