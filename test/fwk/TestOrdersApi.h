//
// Created by Rory McStay on 18/06/2021.
//


#ifndef TRADINGO_TESTORDERSAPI_H
#define TRADINGO_TESTORDERSAPI_H

#include <gtest/gtest.h>
#define _TURN_OFF_PLATFORM_STRING
#include <model/Order.h>
#include <Object.h>
#include <ApiClient.h>

// src
#include "Utils.h"

// test/fwk
#include "TestUtils.h"
#include "Params.h"

using namespace io::swagger::client;

class TestOrdersApi {
private:
    std::map<std::string, std::shared_ptr<model::Order>> _orders;
    std::queue<std::shared_ptr<model::Order>> _orderAmends;
    std::queue<std::shared_ptr<model::Order>> _newOrders;
    std::queue<std::shared_ptr<model::Order>> _orderCancels;
    std::queue<std::shared_ptr<model::ModelBase>> _allEvents;
    long _oidSeed;
    utility::datetime _time;

    // functional helpers
    void add_order(const std::shared_ptr<model::Order>& order_);
    void set_order_timestamp(const std::shared_ptr<model::Order>& order_);

    // API
public:
    TestOrdersApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);

    pplx::task<std::shared_ptr<model::Order>> order_amend(
            std::optional<utility::string_t> orderID,
            std::optional<utility::string_t> origClOrdID,
            std::optional<utility::string_t> clOrdID,
            std::optional<double> simpleOrderQty,
            std::optional<double> orderQty,
            std::optional<double> simpleLeavesQty,
            std::optional<double> leavesQty,
            std::optional<double> price,
            std::optional<double> stopPx,
            std::optional<double> pegOffsetValue,
            std::optional<utility::string_t> text
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_amendBulk(
            std::optional<utility::string_t> orders
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_cancel(
            std::optional<utility::string_t> orderID,
            std::optional<utility::string_t> clOrdID,
            std::optional<utility::string_t> text
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_cancelAll(
            std::optional<utility::string_t> symbol,
            std::optional<utility::string_t> filter,
            std::optional<utility::string_t> text
    );

    pplx::task<std::shared_ptr<model::Object>> order_cancelAllAfter(
            double timeout
    );

    pplx::task<std::shared_ptr<model::Order>> order_closePosition(
            utility::string_t symbol,
            std::optional<double> price
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_getOrders(
            std::optional<utility::string_t> symbol,
            std::optional<utility::string_t> filter,
            std::optional<utility::string_t> columns,
            std::optional<double> count,
            std::optional<double> start,
            std::optional<bool> reverse,
            std::optional<utility::datetime> startTime,
            std::optional<utility::datetime> endTime
    );
    pplx::task<std::shared_ptr<model::Order>> order_new(
            utility::string_t symbol,
            std::optional<utility::string_t> side,
            std::optional<double> simpleOrderQty,
            std::optional<double> orderQty,
            std::optional<double> price,
            std::optional<double> displayQty,
            std::optional<double> stopPx,
            std::optional<utility::string_t> clOrdID,
            std::optional<utility::string_t> clOrdLinkID,
            std::optional<double> pegOffsetValue,
            std::optional<utility::string_t> pegPriceType,
            std::optional<utility::string_t> ordType,
            std::optional<utility::string_t> timeInForce,
            std::optional<utility::string_t> execInst,
            std::optional<utility::string_t> contingencyType,
            std::optional<utility::string_t> text
    );

    pplx::task<std::vector<std::shared_ptr<model::Order>>> order_newBulk(
            std::optional<utility::string_t> orders
    );

    // Testing helpers
private:
    void checkOrderExists(const std::shared_ptr<model::Order>& order);

public:
    void operator >> (const std::string& outEvent_);
    void operator >> (std::vector<std::shared_ptr<model::ModelBase>>& outBuffer_);
    void operator << (utility::datetime time_);

    std::map<std::string, std::shared_ptr<model::Order>>& orders() { return _orders; }
};


#endif //TRADINGO_TESTORDERSAPI_H
