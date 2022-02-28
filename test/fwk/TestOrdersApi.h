//
// Created by Rory McStay on 18/06/2021.
//


#ifndef TRADINGO_TESTORDERSAPI_H
#define TRADINGO_TESTORDERSAPI_H

#include "model/Execution.h"
#include <gtest/gtest.h>
#include <boost/optional.hpp>

#define _TURN_OFF_PLATFORM_STRING
#include <model/Order.h>
#include <model/Margin.h>
#include <Object.h>
#include <ApiClient.h>
#include <model/Position.h>

// src
#include "BatchWriter.h"
#include "Config.h"
#include "Utils.h"

// test/fwk
#include "TestUtils.h"
#include "Params.h"
#include "MarginCalculator.h"

using namespace io::swagger::client;
using namespace tradingo_utils;

class TestOrdersApi {
private:

    /// reference to all orders placed during test, keyed by ClOrdID.
    std::map<std::string, std::shared_ptr<model::Order>> _orders;
    std::queue<std::shared_ptr<model::Order>> _orderAmends;
    std::queue<std::shared_ptr<model::Order>> _rejects;
    std::queue<std::shared_ptr<model::Order>> _newOrders;
    std::queue<std::shared_ptr<model::Order>> _orderCancels;
    std::queue<std::shared_ptr<model::ModelBase>> _allEvents;
    long _oidSeed;
    utility::datetime _time;
    std::shared_ptr<Config> _config;
    std::shared_ptr<MarginCalculator> _marginCalculator;
    std::shared_ptr<TestMarketData> _marketData;

public:
    const std::shared_ptr<MarginCalculator>& getMarginCalculator() const;
    void setMarginCalculator(const std::shared_ptr<MarginCalculator>& marginCalculator);
    using Writer = BatchWriter<std::shared_ptr<model::ModelBase>>;
    const std::shared_ptr<model::Position>& getPosition(const std::string& symbol_) const;
    const std::shared_ptr<model::Margin>& getMargin() const;


    std::queue<std::shared_ptr<model::Order>>& rejects() {return  _rejects; };

    void setMarketData(const std::shared_ptr<TestMarketData>& _marketData);
private:

    // validates and rejects if necessary
    bool validateOrder(const std::shared_ptr<model::Order>& order_);
    std::shared_ptr<model::Order> checkOrderExists(const std::shared_ptr<model::Order>& order);
    bool checkValidAmend(std::shared_ptr<model::Order> amendRequest,
                         std::shared_ptr<model::Order> originalOrder);


public:
    void set_order_timestamp(const std::shared_ptr<model::Order>& order_);
    TestOrdersApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);
    void init(std::shared_ptr<Config> config_);

    // API
    pplx::task<std::shared_ptr<model::Order>> order_amend(
            boost::optional<utility::string_t> orderID,
            boost::optional<utility::string_t> origClOrdID,
            boost::optional<utility::string_t> clOrdID,
            boost::optional<double> simpleOrderQty,
            boost::optional<double> orderQty,
            boost::optional<double> simpleLeavesQty,
            boost::optional<double> leavesQty,
            boost::optional<double> price,
            boost::optional<double> stopPx,
            boost::optional<double> pegOffsetValue,
            boost::optional<utility::string_t> text
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_cancel(
            boost::optional<utility::string_t> orderID,
            boost::optional<utility::string_t> clOrdID,
            boost::optional<utility::string_t> text
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_cancelAll(
            boost::optional<utility::string_t> symbol,
            boost::optional<utility::string_t> filter,
            boost::optional<utility::string_t> text
    );

    pplx::task<std::shared_ptr<model::Object>> order_cancelAllAfter(
            double timeout
    );

    pplx::task<std::shared_ptr<model::Order>> order_closePosition(
            utility::string_t symbol,
            boost::optional<double> price
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_getOrders(
            boost::optional<utility::string_t> symbol,
            boost::optional<utility::string_t> filter,
            boost::optional<utility::string_t> columns,
            boost::optional<double> count,
            boost::optional<double> start,
            boost::optional<bool> reverse,
            boost::optional<utility::datetime> startTime,
            boost::optional<utility::datetime> endTime
    );
    pplx::task<std::shared_ptr<model::Order>> order_new(
            utility::string_t symbol,
            boost::optional<utility::string_t> side,
            boost::optional<double> simpleOrderQty,
            boost::optional<double> orderQty,
            boost::optional<double> price,
            boost::optional<double> displayQty,
            boost::optional<double> stopPx,
            boost::optional<utility::string_t> clOrdID,
            boost::optional<utility::string_t> clOrdLinkID,
            boost::optional<double> pegOffsetValue,
            boost::optional<utility::string_t> pegPriceType,
            boost::optional<utility::string_t> ordType,
            boost::optional<utility::string_t> timeInForce,
            boost::optional<utility::string_t> execInst,
            boost::optional<utility::string_t> contingencyType,
            boost::optional<utility::string_t> text
    );

    // Testing helpers
private:
    bool hasMatchingOrder(const std::shared_ptr<model::Trade>& trade_);

public:
    void operator >> (const std::string& outEvent_);
    void operator >> (std::vector<std::shared_ptr<model::ModelBase>>& outBuffer_);
    void operator >> (Writer& outBuffer_);

    void operator << (const utility::datetime& time_);
    void operator << (const std::shared_ptr<model::Execution>& exec_);

    std::shared_ptr<model::Order> getEvent(const std::string& event_);

    std::map<std::string, std::shared_ptr<model::Order>>& orders() { return _orders; }




};


#endif //TRADINGO_TESTORDERSAPI_H
