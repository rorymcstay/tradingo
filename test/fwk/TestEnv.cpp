#define _TURN_OFF_PLATFORM_STRING
#include <Context.h>
#include "TestEnv.h"


TestEnv::TestEnv(std::initializer_list<std::pair<std::string,std::string>> config_)
:   _config(std::make_shared<Config>(config_))
{
    _config->set("libraryLocation", "/home/user/install/lib/libtest_trading_strategies.so");
    _config->set("baseUrl", "https://localhost:8888/api/v1");
    _config->set("apiKey", "dummy");
    _config->set("apiSecret", "dummy");
    _config->set("connectionString", "https://localhost:8888/realtime");
    _config->set("clOrdPrefix", "MCST");
    _context = std::make_shared<Context<TestMarketData, OrderApi>>(_config);
    _context->init();
}

void TestEnv::operator<<(const std::string &value_) {
    try {
        *_context->marketData() << value_;
        _context->strategy()->evaluate();
    } catch (std::runtime_error& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event <<\n\n      " << value_;
    }
}

void TestEnv::operator>>(const std::string &value_) {
    try {
        *_context->orderApi() >> value_;
    } catch (std::runtime_error& ex) {
        FAIL() << "TEST Exception: " << ex.what() << " during event >>\n\n      " << value_;
    }
}
