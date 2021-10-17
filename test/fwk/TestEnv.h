//
// Created by Rory McStay on 22/06/2021.
//

#ifndef TRADINGO_TESTENV_H
#define TRADINGO_TESTENV_H
#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "TestMarketData.h"
#include "TestOrdersApi.h"

#include "Strategy.h"
#include "OrderInterface.h"
#include "Utils.h"
#include "Context.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define FILENAME(x) __FILENAME__#x
#define LN  " | " __FILE__ ":" TO_STRING(__LINE__)

using namespace io::swagger::client;

struct Dispatch {
    utility::datetime mkt_time;
    utility::datetime actual_time;
};

#define DEFAULT_ARGS \
        {"symbol", "XBTUSD"},\
        {"clOrdPrefix", "MCST"},\
        {"factoryMethod", "RegisterBreakOutStrategy"},\
        {"startingAmount", "1000"},\
        {"displaySize", "200"},\
        {"referencePrice", "35000"},\
        {"shortTermWindow", "1000"},\
        {"longTermWindow", "8000"},\
        {"moving_average_crossover-interval", "1000"},\
        {"signal-callback", "1000"},\
        {"logLevel", "debug"},\
        {"moving_average_crossover-callback", "false"}, \
        {"realtime", "false"},                         \
        {"override-signal-callback", "true"},            \
        {"libraryLocation", LIBRARY_LOCATION"/libtest_trading_strategies.so" }, \
        {"storage", "./"},   \
        {"tickStorage", "./"}

#define INSERT_DEFAULT_ARGS(config_) \
    std::initializer_list<std::pair<std::string, std::string>> defaults = { DEFAULT_ARGS }; \
    for (auto pair : defaults) \
        (config_)->set(pair.first, pair.second);

class TestEnv
{
    using OrderApi = TestOrdersApi;
    using TStrategy = Strategy<OrderApi>;

    std::shared_ptr<Config> _config;
    std::shared_ptr<Context<TestMarketData, OrderApi>> _context;
    std::shared_ptr<model::Position> _position;
    Dispatch _lastDispatch; // first is time, second is mkttime
    bool _realtime;
    long _events;
public:
    const std::shared_ptr<TStrategy>& strategy() const { return _context->strategy(); }
    const std::shared_ptr<Context<TestMarketData, OrderApi>>& context() const { return _context; }
    TestEnv(std::initializer_list<std::pair<std::string, std::string>>);
    TestEnv(const std::shared_ptr<Config>& config_);

    void init();

    void playback(const std::string& tradeFile_, const std::string& quoteFile_);

    void dispatch(utility::datetime time, const std::shared_ptr<model::Quote>& quote,
                  const std::shared_ptr<model::Execution> exec_, const std::shared_ptr<model::Order> order_);

    void operator << (const std::string& value_);
    void operator >> (const std::string& value_);
    void sleep(const utility::datetime& time_) const;
};
template<typename T> std::shared_ptr<T> getEvent(std::ifstream& fileHandle_);


#endif //TRADINGO_TESTENV_H
