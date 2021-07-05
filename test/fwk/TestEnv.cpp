#include "TestEnv.h"


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _marketDataProvider(std::make_shared<TestMarketData>())
,   _orderApi(std::make_shared<OrderApi>())
,   _config(std::make_shared<Config>(config_))
,   _strategy(std::make_shared<TStrategy>(_marketDataProvider, _orderApi)) {
    _strategy->init(_config);
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *_marketDataProvider << value_;
        _strategy->evaluate();
    } catch (std::runtime_error& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event <<\n\n      " << value_;
    }
}

void TestEnv::operator>>(const std::string &value_) {
    try {
        *_orderApi >> value_;
    } catch (std::runtime_error& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event >>\n\n      " << value_;
    }
}
