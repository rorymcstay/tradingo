//
// Created by Rory McStay on 18/06/2021.
//

#define _TURN_OFF_PLATFORM_STRING
#include <Object.h>
#include <Allocation.h>
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
TestOrdersApi::order_amend(boost::optional<utility::string_t> orderID, boost::optional<utility::string_t> origClOrdID,
                           boost::optional<utility::string_t> clOrdID, boost::optional<double> simpleOrderQty,
                           boost::optional<double> orderQty, boost::optional<double> simpleLeavesQty,
                           boost::optional<double> leavesQty, boost::optional<double> price,
                           boost::optional<double> stopPx, boost::optional<double> pegOffsetValue,
                           boost::optional<utility::string_t> text) {
    auto order = std::make_shared<model::Order>();
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
TestOrdersApi::order_amendBulk(boost::optional<utility::string_t> orders) {

    auto outOrders = std::vector<std::shared_ptr<model::Order>>();
    auto json = web::json::value::parse(orders.value()).as_array();
    for (auto& ordJson : json) {
        auto order = std::make_shared<model::Order>();
        order->fromJson(ordJson);
        auto origOrder = checkOrderExists(order);
        // checkValidAmend(order);
        order->setLeavesQty(order->getOrderQty() - origOrder->getCumQty());
        order->setCumQty(origOrder->getCumQty());
        _orders[order->getClOrdID()] = order;
        set_order_timestamp(order);
        _orders.erase(order->getOrigClOrdID());
        validateOrder(order);
        order->setOrdStatus("New");
        _orderAmends.emplace(order);
        outOrders.push_back(order);
    }
    auto tsk = pplx::task_from_result(outOrders);
    return tsk;
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancel(boost::optional<utility::string_t> orderID,
                            boost::optional<utility::string_t> clOrdID,
                            boost::optional<utility::string_t> text) {
    std::vector<std::shared_ptr<model::Order>> ordersRet;

    if (_orders.find(clOrdID.value()) == _orders.end()) {
        LOGWARN(LOG_NVP("ClOrdID",clOrdID.value()) << " not found to cancel.");
        _orderCancels.push(nullptr);
    } else {
        _orders[clOrdID.value()]->setOrdStatus("PendingCancel");
        _orders[clOrdID.value()]->setOrderQty(0.0);
        set_order_timestamp(_orders[clOrdID.value()]);
        //validateOrder(_orders[orderID.value()]);
        ordersRet.push_back(_orders[clOrdID.value()]);
        _orderCancels.push(_orders[clOrdID.value()]);
        _allEvents.push(_orders[clOrdID.value()]);
        _orders.erase(clOrdID.value());

    }
    return pplx::task_from_result(ordersRet);
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancelAll(boost::optional<utility::string_t> symbol, boost::optional<utility::string_t> filter,
                               boost::optional<utility::string_t> text) {
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
TestOrdersApi::order_closePosition(utility::string_t symbol, boost::optional<double> price) {
    return pplx::task_from_result(std::make_shared<model::Order>());
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_getOrders(boost::optional<utility::string_t> symbol, boost::optional<utility::string_t> filter,
                               boost::optional<utility::string_t> columns, boost::optional<double> count,
                               boost::optional<double> start, boost::optional<bool> reverse,
                               boost::optional<utility::datetime> startTime,
                               boost::optional<utility::datetime> endTime) {
    auto orders = std::vector<std::shared_ptr<model::Order>>();
    for (auto& ord : _orders) {
        orders.push_back(ord.second);
    }
    return pplx::task_from_result(orders);
}

pplx::task<std::shared_ptr<model::Order>>
TestOrdersApi::order_new(utility::string_t symbol, boost::optional<utility::string_t> side,
                         boost::optional<double> simpleOrderQty, boost::optional<double> orderQty,
                         boost::optional<double> price, boost::optional<double> displayQty,
                         boost::optional<double> stopPx, boost::optional<utility::string_t> clOrdID,
                         boost::optional<utility::string_t> clOrdLinkID, boost::optional<double> pegOffsetValue,
                         boost::optional<utility::string_t> pegPriceType, boost::optional<utility::string_t> ordType,
                         boost::optional<utility::string_t> timeInForce, boost::optional<utility::string_t> execInst,
                         boost::optional<utility::string_t> contingencyType, boost::optional<utility::string_t> text) {
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
    if (clOrdID.has_value()) {
        order->setClOrdID(clOrdID.value());
    }
    add_order(order);
    validateOrder(order);

    return pplx::task_from_result(order);
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_newBulk(boost::optional<utility::string_t> orders) {
    auto outOrders = std::vector<std::shared_ptr<model::Order>>();
    auto json = web::json::value::parse(orders.value()).as_array();
    for (auto& ordJson : json) {
        ordJson.as_object()["orderID"] = web::json::value::string(std::to_string(++_oidSeed));
        auto order = std::make_shared<model::Order>();
        order->setClOrdID(ordJson.at("clOrdID").as_string());
        order->fromJson(ordJson);
        add_order(order);
        validateOrder(order);
        /// TODO this is not atomic some orders will be placed?
        outOrders.push_back(order);
    }
    auto tsk = pplx::task_from_result(outOrders);
    return tsk;
}

#define CHECK_VAL(val1, val2)                                                                  \
    if (val1 != val2) {                                                                        \
        std::stringstream ss;                                                                  \
        ss << "The values " << LOG_NVP(#val1, val1) << " and " << LOG_NVP(#val2, val2) << " ";\
        throw std::runtime_error(ss.str());                                                    \
    }

void TestOrdersApi::operator>>(const std::string &outEvent_) {
    auto eventType = getEventTypeFromString(outEvent_);
    if (eventType == "NONE") {
        if (!_newOrders.empty() || !_orderCancels.empty() || !_orderAmends.empty()) {
            auto message = [](const std::shared_ptr<model::ModelBase>& ptr_ ) { return ptr_->toJson().serialize(); };
            std::stringstream failMessage;
            failMessage << "ORDER_NEW:\n";
            while (!_newOrders.empty()) {
                failMessage << message(_newOrders.front()) << "\n";
                _newOrders.pop();
            }
            failMessage << "ORDER_AMEND:\n";
            while (!_orderAmends.empty()) {
                failMessage << message(_orderAmends.front()) << "\n";
                _orderAmends.pop();
            }
            failMessage << "ORDER_CANCEL:\n";
            while (!_orderCancels.empty()) {
                failMessage << message(_orderCancels.front()) << "\n";
                _orderCancels.pop();
            }
            throw std::runtime_error("There are events still pending!\n"+failMessage.str());
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

std::shared_ptr<model::Order> TestOrdersApi::checkOrderExists(const std::shared_ptr<model::Order> &order) {
    // hack storing orders under both id's.
    if (_orders.find(order->getClOrdID()) != _orders.end()
        || _orders.find(order->getOrigClOrdID()) != _orders.end()) {
        // is amend
        return _orders.find(order->getClOrdID()) != _orders.end() ? _orders[order->getClOrdID()] : _orders[order->getOrigClOrdID()];
        /*assert((
                _orders[order->getClOrdID()]->getOrdStatus() == "PendingCancel"
             || _orders[order->getClOrdID()]->getOrdStatus() == "PendingAmend"
             || _orders[order->getClOrdID()]->getOrdStatus() == "New")
                && "Unhandled condition for TestOrdersApi::checkOrderExists ClOrdID check")*/
    } else {
        // reject amend request, fail test
        std::stringstream str;
        str << "Original order " << LOG_NVP("OrderID",order->getOrderID())
            << LOG_NVP("ClOrdID", order->getClOrdID())
            << LOG_NVP("OrigClOrdID", order->getOrigClOrdID()) << " not found.";
        throw std::runtime_error(str.str());
    }

}

void TestOrdersApi::add_order(const std::shared_ptr<model::Order> &order_) {
    _oidSeed++;
    order_->setOrderID(std::to_string(_oidSeed));
    _orders[order_->getClOrdID()] = order_;
    order_->setOrdStatus("New");
    order_->setLeavesQty(order_->getOrderQty());
    order_->setCumQty(0.0);
    set_order_timestamp(order_);
    _newOrders.push(order_);
    _allEvents.push(order_);

}

void TestOrdersApi::operator>>(std::vector<std::shared_ptr<model::ModelBase>>& outVec) {
    while (!_allEvents.empty()){
        auto top = _allEvents.front();
        auto val = top->toJson();
        LOGINFO(AixLog::Color::GREEN << "TestOrdersApi::OUT>> " << AixLog::Color::GREEN << val.serialize() << AixLog::Color::none);
        val.as_object()["timestamp"] = web::json::value(_time.to_string());
        top->fromJson(val);
        outVec.push_back(top);
        _allEvents.pop();
    }
}

void TestOrdersApi::operator>>(BatchWriter& outVec) {
    while (!_allEvents.empty()){
        auto top = _allEvents.front();
        auto val = top->toJson();
        LOGINFO(AixLog::Color::GREEN << "TestOrdersApi::OUT>> " << AixLog::Color::GREEN << val.serialize() << AixLog::Color::none);
        val.as_object()["timestamp"] = web::json::value(_time.to_string());
        top->fromJson(val);
        outVec.write(top);
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

bool TestOrdersApi::hasMatchingOrder(const std::shared_ptr<model::Trade>& trade_) {

    for (auto& order : _orders) {
        if ((order.second->getPrice() >= trade_->getPrice() && order.second->getSide() == "Buy")
            || order.second->getPrice() <= trade_->getPrice() && order.second->getSide() == "Sell") {
            // we may cross our order on the trade
            return true;
        }
    }
    return false;

}

void TestOrdersApi::addExecToPosition(const std::shared_ptr<model::Execution>& exec_) {

    qty_t newPosition = _position->getCurrentQty() + exec_->getLastQty();
    qty_t newCost = _position->getCurrentCost() + (exec_->getPrice() * exec_->getLastQty());
    _position->setCurrentQty(newPosition);
    _position->setCurrentCost(newCost);
}

const std::shared_ptr<model::Position> &TestOrdersApi::getPosition() const {
    return _position;
}

void TestOrdersApi::setPosition(const std::shared_ptr<model::Position> &position) {
    _position = position;
}

// TODO validate and add order
// TODO assert clordid is unique.
bool TestOrdersApi::validateOrder(const std::shared_ptr<model::Order> &order_) {
    if (order_->origClOrdIDIsSet() && order_->getOrderID() != "") {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        auto error = api::ApiException(400, "Must specify only one of orderID or origClOrdId", content);
        throw error;
    }
    if (order_->origClOrdIDIsSet() && !order_->clOrdIDIsSet()){
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        auto error = api::ApiException(400, "Must specify ClOrdID if OrigClOrdID is set", content);
        throw error;
    }
    if ((int)order_->getOrderQty() % std::stoi(_config->get("lotSize", "100")) != 0) {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(400, "Orderty is not a multiple of lotsize", content);
    }
    return true;
}

void TestOrdersApi::init(std::shared_ptr<Config> config_) {
    _config = config_;
}


