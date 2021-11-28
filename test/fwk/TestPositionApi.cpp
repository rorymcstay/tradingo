#include "TestPositionApi.h"

TestPositionApi::TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr) {

}

void TestPositionApi::position_updateLeverage(const std::string& symbol_, double leverage_) {
    _leverages[symbol_] = leverage_;
}
