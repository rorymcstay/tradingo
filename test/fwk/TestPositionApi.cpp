#include "TestPositionApi.h"
#include "ApiException.h"

TestPositionApi::TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr) {

}

void TestPositionApi::position_updateLeverage(const std::string& symbol_, double leverage_) {
    if (_positions.find(symbol_) != _positions.end()) {
        _positions.at(symbol_)->setLeverage(leverage_);
        return;
    }
    std::stringstream str;
    auto content = std::make_shared<std::istringstream>(
        R"({"status": "error", "message": "No position for symbol", "symbol": ")" + symbol_ + R"("})");
    str << "No position for symbol " + symbol_;
    throw api::ApiException(404, str.str(), content);
}
