#include "TestEnv.h"


TestEnv::TestEnv(const std::string &config_)
:   _marketDataProvider(std::make_shared<TestMarketData>())
,   _orderApi(std::make_shared<OrderApi>())
,   _strategy(std::make_shared<TStrategy>(_marketDataProvider, _orderApib)) {
    _strategy->init(config_);
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *_marketDataProvider << value_;
    } catch (std::runtime_error& ex) {
        FAIL() << value_ << "\nException: " << ex.what();
    }
    _strategy->evaluate();
}

void TestEnv::operator>>(const std::string &value_) {
    try {
        _orderApi->exchange() >> value_;
    } catch (std::runtime_error& ex) {
        FAIL() << value_ << "\nException: " << ex.what();
    }
}
