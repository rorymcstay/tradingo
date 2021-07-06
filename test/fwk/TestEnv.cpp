#define _TURN_OFF_PLATFORM_STRING
#include <Context.h>
#include "TestEnv.h"


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _config(std::make_shared<Config>(config_))
//,   _strategy(std::make_shared<TStrategy>(_marketDataProvider, _orderApi))
{
    _config->set("libraryLocation", "/home/user/install/lib/libtest_trading_strategies.a");
    _context = std::make_shared<Context<TestMarketData, OrderApi>>(_config);
    auto method = _context->loadFactoryMethod();
    method(_marketDataProvider, _orderApi);
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *(_context->marketData()) << value_;
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
