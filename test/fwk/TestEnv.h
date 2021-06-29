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

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define FILENAME(x) __FILENAME__#x
#define LN  " | " __FILE__ ":" TO_STRING(__LINE__)

using namespace io::swagger::client;

class TestEnv
{
    using OrderApi = TestOrdersApi;
    using TStrategy = Strategy<OrderApi>;

    std::shared_ptr<TestMarketData> _marketDataProvider;
    std::shared_ptr<OrderApi> _orderApi;
    std::shared_ptr<TStrategy> _strategy;

public:
    explicit TestEnv(const std::string& config_);
    void operator << (const std::string& value_);
    void operator >> (const std::string& value_);
};


#endif //TRADINGO_TESTENV_H