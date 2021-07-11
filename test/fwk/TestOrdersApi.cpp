//
// Created by Rory McStay on 18/06/2021.
//

#define _TURN_OFF_PLATFORM_STRING
#include <Object.h>
#include "TestOrdersApi.h"

TestOrdersApi::TestOrdersApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr)
:   _orders()
,   _newOrders()
,   _orderAmends()
,   _orderCancels()
,   _oidSeed(0) {

    LOGINFO("TestOrdersApi initialised!");

}

pplx::task<std::shared_ptr<model::Order>>
TestOrdersApi::order_amend(std::optional<utility::string_t> orderID, std::optional<utility::string_t> origClOrdID,
                           std::optional<utility::string_t> clOrdID, std::optional<double> simpleOrderQty,
                           std::optional<double> orderQty, std::optional<double> simpleLeavesQty,
                           std::optional<double> leavesQty, std::optional<double> price,
                           std::optional<double> stopPx, std::optional<double> pegOffsetValue,
                           std::optional<utility::string_t> text) {
    auto order = std::make_shared<model::Order>();
    order->setOrderID(orderID.value());
    order->setClOrdID(origClOrdID.value());
    order->setOrderQty(simpleOrderQty.value());
    order->setPrice(price.value());
    order->setStopPx(stopPx.value());
    set_order_timestamp(order);
    _orderAmends.emplace(order);
    auto task = pplx::task_from_result(order);
    //auto task = pplx::task<decltype(order)>(order);
    _allEvents.push(order);
    return task;
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_amendBulk(std::optional<utility::string_t> orders) {

    auto outOrders = std::vector<std::shared_ptr<model::Order>>();
    auto json = web::json::value::parse(orders.value()).as_array();
    for (auto& ordJson : json) {
        auto order = std::make_shared<model::Order>();
        order->fromJson(ordJson);
        checkOrderExists(order);
        set_order_timestamp(order);
        _orderAmends.emplace(order);
        outOrders.push_back(order);
    }
    auto tsk = pplx::task_from_result(outOrders);
    return tsk;
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancel(std::optional<utility::string_t> orderID,
                            std::optional<utility::string_t> clOrdID,
                            std::optional<utility::string_t> text) {
    std::vector<std::shared_ptr<model::Order>> ordersRet;
    if (_orders.find(orderID.value()) == _orders.end()) {
        LOGWARN(LOG_NVP("OrderID",orderID.value()) << " not found to cancel.");
        _orderCancels.push(nullptr);
    } else {
        _orders[orderID.value()]->setOrdStatus("PendingCancel");
        _orders[orderID.value()]->setOrderQty(0.0);
        set_order_timestamp(_orders[orderID.value()]);
        ordersRet.push_back(_orders[orderID.value()]);
        _orderCancels.push(_orders[orderID.value()]);
        _allEvents.push(_orders[orderID.value()]);
    }
    return pplx::task_from_result(ordersRet);
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancelAll(std::optional<utility::string_t> symbol, std::optional<utility::string_t> filter,
                               std::optional<utility::string_t> text) {
    std::vector<std::shared_ptr<model::Order>> out = {};
    for (auto& orders : _orders) {
        orders.second->setOrdStatus("PendingCancel");
        _orderCancels.push(orders.second);
        out.push_back(orders.second);
        _allEvents.push(orders.second);
        set_order_timestamp(orders.second);
    }
    return pplx::task_from_result(out);
}

pplx::task<std::shared_ptr<model::Object>> TestOrdersApi::order_cancelAllAfter(double timeout) {
    return pplx::task_from_result(std::make_shared<model::Object>());
}

pplx::task<std::shared_ptr<model::Order>>
TestOrdersApi::order_closePosition(utility::string_t symbol, std::optional<double> price) {
    return pplx::task_from_result(std::make_shared<model::Order>());
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_getOrders(std::optional<utility::string_t> symbol, std::optional<utility::string_t> filter,
                               std::optional<utility::string_t> columns, std::optional<double> count,
                               std::optional<double> start, std::optional<bool> reverse,
                               std::optional<utility::datetime> startTime,
                               std::optional<utility::datetime> endTime) {
    auto orders = std::vector<std::shared_ptr<model::Order>>();
    for (auto& ord : _orders) {
        orders.push_back(ord.second);
    }
    return pplx::task_from_result(orders);
}

pplx::task<std::shared_ptr<model::Order>>
TestOrdersApi::order_new(utility::string_t symbol, std::optional<utility::string_t> side,
                         std::optional<double> simpleOrderQty, std::optional<double> orderQty,
                         std::optional<double> price, std::optional<double> displayQty,
                         std::optional<double> stopPx, std::optional<utility::string_t> clOrdID,
                         std::optional<utility::string_t> clOrdLinkID, std::optional<double> pegOffsetValue,
                         std::optional<utility::string_t> pegPriceType, std::optional<utility::string_t> ordType,
                         std::optional<utility::string_t> timeInForce, std::optional<utility::string_t> execInst,
                         std::optional<utility::string_t> contingencyType, std::optional<utility::string_t> text) {
    auto order = std::make_shared<model::Order>();
    order->setOrdStatus("New");
    if (side.has_value())
        order->setSide(side.value());
    if (stopPx.has_value())
        order->setStopPx(stopPx.value());
    if (simpleOrderQty.has_value())
        order->setOrderQty(simpleOrderQty.value());
    if (price.has_value())
        order->setPrice(price.value());
    order->setOrderID(std::to_string(_oidSeed++));

    add_order(order);
    return pplx::task_from_result(order);
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_newBulk(std::optional<utility::string_t> orders) {
    auto outOrders = std::vector<std::shared_ptr<model::Order>>();
    auto json = web::json::value::parse(orders.value()).as_array();
    for (auto& ordJson : json) {
        ordJson.as_object()["orderID"] = web::json::value::string(std::to_string(++_oidSeed));
        auto order = std::make_shared<model::Order>();
        add_order(order);
        order->fromJson(ordJson);
        outOrders.push_back(order);
    }
    auto tsk = pplx::task_from_result(outOrders);
    return tsk;
}

#define CHECK_VAL(val1, val2)                                                                  \
    if (val1 != val2) {                                                                        \
        std::stringstream ss;                                                                  \
        ss << "The values " <<  LOG_NVP(#val1, val1) << " and " << LOG_NVP(#val2, val2) << " ";\
        throw std::runtime_error(ss.str());                                                    \
    }

void TestOrdersApi::operator>>(const std::string &outEvent_) {
    auto eventType = getEventTypeFromString(outEvent_);
    if (eventType == "NONE") {
        if (!_newOrders.empty() || !_orderCancels.empty() || !_orderAmends.empty()) {
            throw std::runtime_error("There are events still pending!" );
        }
        return;
    }
    auto params = Params(outEvent_);
    auto expectedOrder = fromJson<model::Order>(params.asJson());
    if (eventType == "ORDER_NEW") {
        if (_newOrders.empty()) {
            throw std::runtime_error("No new orders created.");
        }
        auto latestOrder = _newOrders.front();
        CHECK_VAL(latestOrder->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(latestOrder->getSide(), expectedOrder->getSide());
        CHECK_VAL(latestOrder->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(latestOrder->getSymbol(), expectedOrder->getSymbol());
        _newOrders.pop();
    } else if (eventType == "ORDER_AMEND") {
        checkOrderExists(expectedOrder);
        if (_orderAmends.empty()) {
            throw std::runtime_error("No amends made for any orders");
        }
        auto orderAmend = _orderAmends.front();
        _orderAmends.pop();
        CHECK_VAL(orderAmend->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(orderAmend->getSide(), expectedOrder->getSide());
        CHECK_VAL(orderAmend->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(orderAmend->getSymbol(), expectedOrder->getSymbol());
    } else if (eventType == "ORDER_CANCEL") {
        checkOrderExists(expectedOrder);
        if (_orderCancels.empty()) {
            throw std::runtime_error( "No expectedOrder cancels");
        }
        auto orderCancel = _orderCancels.front();
        _orderCancels.pop();
        CHECK_VAL(orderCancel->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(orderCancel->getSide(), expectedOrder->getSide());
        CHECK_VAL(orderCancel->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(orderCancel->getSymbol(), expectedOrder->getSymbol());
    } else {
        throw std::runtime_error("Must specify event type - One of ORDER_NEW, ORDER_AMEND, ORDER_CANCEL");
    }
}

void TestOrdersApi::checkOrderExists(const std::shared_ptr<model::Order> &order) {
    if (_orders.find(order->getOrderID()) == _orders.end()) {
        // reject amend request, fail test
        std::stringstream str;
        str << "Original order " << LOG_VAR(order->getOrderID()) << " not found.";
        throw std::runtime_error(str.str());
    }
}

void TestOrdersApi::add_order(const std::shared_ptr<model::Order> &order_) {
    order_->setOrderID(std::to_string(_oidSeed));
    _orders[order_->getOrderID()] = order_;
    order_->setOrdStatus("New");
    set_order_timestamp(order_);
    _newOrders.push(order_);
    _allEvents.push(order_);

}

void TestOrdersApi::operator>>(std::vector<std::shared_ptr<model::ModelBase>>& outVec) {
    while (!_allEvents.empty()){
        auto top = _allEvents.front();
        auto val = top->toJson();
        val.as_object()["timestamp"] = web::json::value(_time.to_string());
        top->fromJson(val);
        outVec.push_back(top);
        _allEvents.pop();
    }

}

void TestOrdersApi::operator<<(utility::datetime time_) {
    _time = time_;
}

void TestOrdersApi::set_order_timestamp(const std::shared_ptr<model::Order>& order_) {

    if (!order_->timestampIsSet()) {
        if (!_time.is_initialized()){
            order_->setTimestamp(_time);
        } else {
            order_->setTimestamp(utility::datetime::utc_now());
        }
    }
}


