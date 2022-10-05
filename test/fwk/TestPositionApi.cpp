#include "TestPositionApi.h"
#include "ApiException.h"

TestPositionApi::TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr) {

}


void TestPositionApi::setMarketData(const std::shared_ptr<TestMarketData>& marketData_) {
    _marketData = marketData_;
}


void TestPositionApi::position_updateLeverage(const std::string& symbol_, double leverage_) {
    if (_marketData->getPositions().find(symbol_) != _positions.end()) {
        auto position = _marketData->getPositions().at(symbol_);
        position->setLeverage(leverage_);
        position->setInitMarginReq(1/leverage_);
        return;
    }
    std::stringstream str;
    auto content = std::make_shared<std::istringstream>(
        R"({"status": "error", "message": "No position for symbol", "symbol": ")" + symbol_ + R"("})");
    str << "No position for symbol " + symbol_;
    throw api::ApiException(404, str.str(), content);
}
