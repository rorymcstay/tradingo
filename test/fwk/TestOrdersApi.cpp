//
// Created by Rory McStay on 18/06/2021.
//

#include <stdexcept>
#define _TURN_OFF_PLATFORM_STRING
#include <Object.h>
#include <Allocation.h>
#include <model/Margin.h>
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
    order->setOrdStatus("Replaced");
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
        order->setOrdStatus("Replaced");
        _allEvents.push(order);
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
        _orders[clOrdID.value()]->setOrdStatus("Canceled");
        _orders[clOrdID.value()]->setOrderQty(0.0);
        _orders[clOrdID.value()]->setLeavesQty(0.0);
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
        orders.second->setOrdStatus("Canceled");
        _orderCancels.push(orders.second);
        out.push_back(orders.second);
        set_order_timestamp(orders.second);
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

#define PVAR(order, name_)  #name_ "="  << (order)->get##name_() << " "
std::shared_ptr<model::Order> TestOrdersApi::operator>>(const std::string &outEvent_) {
    auto eventType = getEventTypeFromString(outEvent_);

    std::stringstream failMessage;
    failMessage << "Event filter:\n\t" << outEvent_ << "\nnot satisifed. Reason:\n";
    if (eventType == "NONE") {
        if (!_newOrders.empty() || !_orderCancels.empty() || !_orderAmends.empty()) {
            failMessage << "There are events still pending!\n";
            while (!_newOrders.empty()) {
                auto order = _newOrders.front();
                failMessage << "env >> \"ORDER_NEW "
                            << PVAR(order, Price)
                            << PVAR(order, OrderQty)
                            << PVAR(order, CumQty)
                            << PVAR(order, LeavesQty)
                            << PVAR(order, OrderID)
                            << PVAR(order, ClOrdID)
                            << PVAR(order, OrigClOrdID)
                            << PVAR(order, OrdStatus)
                            << PVAR(order, Side)
                            << PVAR(order, Symbol)
                        << "\" LN;\n";
                _newOrders.pop();
            }
            while (!_orderAmends.empty()) {
                auto order = _orderAmends.front();
                failMessage << "env >> \"ORDER_AMEND "
                            << PVAR(order, Price)
                            << PVAR(order, OrderQty)
                            << PVAR(order, CumQty)
                            << PVAR(order, LeavesQty)
                            << PVAR(order, OrderID)
                            << PVAR(order, ClOrdID)
                            << PVAR(order, OrigClOrdID)
                            << PVAR(order, OrdStatus)
                            << PVAR(order, Side)
                            << PVAR(order, Symbol)

                        << "\" LN;\n";
                _orderAmends.pop();
            }
            while (!_orderCancels.empty()) {
                auto order = _orderCancels.front();
                failMessage << "env >> \"ORDER_CANCEL "
                            << PVAR(order, Price)
                            << PVAR(order, OrderQty)
                            << PVAR(order, CumQty)
                            << PVAR(order, LeavesQty)
                            << PVAR(order, OrderID)
                            << PVAR(order, ClOrdID)
                            << PVAR(order, OrigClOrdID)
                            << PVAR(order, OrdStatus)
                            << PVAR(order, Side)
                            << PVAR(order, Symbol)
                        << "\" LN;\n";
                _orderCancels.pop();
            }
        }
        return nullptr;
    }
    auto params = Params(outEvent_);
    auto expectedOrder = fromJson<model::Order>(params.asJson());
    if (eventType == "ORDER_NEW") {
        if (_newOrders.empty()) {
            failMessage << "No new orders created.";
        }
        auto latestOrder = _newOrders.front();
        CHECK_VAL(latestOrder->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(latestOrder->getSide(), expectedOrder->getSide());
        CHECK_VAL(latestOrder->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(latestOrder->getSymbol(), expectedOrder->getSymbol());
        _newOrders.pop();
        return latestOrder;
    } else if (eventType == "ORDER_AMEND") {
        checkOrderExists(expectedOrder);
        if (_orderAmends.empty()) {
            failMessage << "No amends made for any orders";
        }
        auto orderAmend = _orderAmends.front();
        _orderAmends.pop();
        CHECK_VAL(orderAmend->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(orderAmend->getSide(), expectedOrder->getSide());
        CHECK_VAL(orderAmend->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(orderAmend->getSymbol(), expectedOrder->getSymbol());
        return orderAmend;
    } else if (eventType == "ORDER_CANCEL") {
        if (_orderCancels.empty()) {
            failMessage << "No expectedOrder cancels";
        }
        auto orderCancel = _orderCancels.front();
        _orderCancels.pop();
        CHECK_VAL(orderCancel->getPrice(), expectedOrder->getPrice());
        CHECK_VAL(orderCancel->getSide(), expectedOrder->getSide());
        CHECK_VAL(orderCancel->getOrderQty(), expectedOrder->getOrderQty());
        CHECK_VAL(orderCancel->getSymbol(), expectedOrder->getSymbol());
        return orderCancel;
    }
    failMessage << "Must specify event type - One of \"ORDER_NEW\", \"ORDER_AMEND\", \"ORDER_CANCEL\"";
    throw std::runtime_error(failMessage.str());
}

std::shared_ptr<model::Order> TestOrdersApi::checkOrderExists(const std::shared_ptr<model::Order> &order) {
    // hack storing orders under both id's.
    if (_orders.find(order->getClOrdID()) != _orders.end()
        || _orders.find(order->getOrigClOrdID()) != _orders.end()) {
        // is amend
        return _orders.find(order->getClOrdID()) != _orders.end() ? _orders[order->getClOrdID()] : _orders[order->getOrigClOrdID()];
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
        top->fromJson(val);
        outVec.push_back(top);
        _allEvents.pop();
    }
}

void TestOrdersApi::operator>>(TestOrdersApi::Writer & outVec) {
    while (!_allEvents.empty()){
        auto top = _allEvents.front();
        auto val = top->toJson();
        //val.as_object()["timestamp"] = web::json::value(_time.to_string());
        LOGDEBUG(AixLog::Color::GREEN << "TestOrdersApi::OUT>> " << AixLog::Color::GREEN << val.serialize() << AixLog::Color::none);
        top->fromJson(val);
        outVec.write(top);
        _allEvents.pop();
    }
}
void TestOrdersApi::operator<<(const utility::datetime& time_) {
    _time = time_;
}

void TestOrdersApi::set_order_timestamp(const std::shared_ptr<model::Order>& order_) {

    if (!order_->timestampIsSet()) {
        if (_time.is_initialized()){
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
    LOGINFO(AixLog::Color::GREEN
                    << "OnTrade: "
                    << LOG_NVP("ClOrdID", exec_->getClOrdID())
                    << LOG_NVP("OrderID", exec_->getOrderID())
                    << LOG_NVP("Side", exec_->getSide())
                    << LOG_NVP("OrderQty", exec_->getOrderQty())
                    << LOG_NVP("Price", exec_->getPrice())
                    << LOG_NVP("LastQty", exec_->getLastQty())
                    << LOG_NVP("LastPx", exec_->getLastPx())
                    << AixLog::Color::none);

    auto lastQty = exec_->getLastQty();
    auto lastPx = exec_->getLastPx();
    auto dirMx = (exec_->getSide() == "Buy") ? -1 : +1;
    auto cost = lastQty * lastPx *dirMx;
    auto newBalance = cost + _margin->getWalletBalance();
    _margin->setWalletBalance(newBalance);
    auto marginBalance = newBalance + _position->getUnrealisedPnl();
    _margin->setMarginBalance(marginBalance);
    double maintenanceMargin = _marginCalculator->getMarginAmount(_position);
    _position->setMaintMarginReq(maintenanceMargin);
    _margin->setMaintMargin(maintenanceMargin);

    qty_t newPosition = _position->getCurrentQty() + ((exec_->getSide() == "Buy") ? exec_->getLastQty() : -exec_->getLastQty());
    qty_t newCost = _position->getCurrentCost() + ((1/exec_->getPrice() * exec_->getLastQty())*(exec_->getSide()=="Buy" ? 1 : -1));
    _position->setCurrentQty(newPosition);
    _position->setCurrentCost(newCost);
    auto unrealisedPnl = _marginCalculator->getUnrealisedPnL(_position);
    _position->setUnrealisedPnl(unrealisedPnl);
    _position->setTimestamp(exec_->getTimestamp());
    auto liqPrice = _marginCalculator->getLiquidationPrice(_position, newBalance);
    _position->setLiquidationPrice(liqPrice);
    LOGINFO(AixLog::Color::GREEN
                    << "New Position: "
                    << LOG_NVP("CurrentCost", _position->getCurrentCost())
                    << LOG_NVP("Size", _position->getCurrentQty())
                    << LOG_NVP("Time", _position->getTimestamp().to_string())
                    << AixLog::Color::none);
}

void TestOrdersApi::addExecToMargin(const std::shared_ptr<model::Execution>& exec_) {


}

const std::shared_ptr<model::Position> &TestOrdersApi::getPosition() const {
    return _position;
}

void TestOrdersApi::setPosition(const std::shared_ptr<model::Position> &position) {
    _position = position;
}

const std::shared_ptr<model::Margin>& TestOrdersApi::getMargin() const {
    return _margin;
}
void TestOrdersApi::setMargin(const std::shared_ptr<model::Margin> &margin_) {
    _margin = margin_;
}

/*
 * Check order is valid given current status of margin and that parameters are correct.
 */
bool TestOrdersApi::validateOrder(const std::shared_ptr<model::Order> &order_) {
    if (order_->origClOrdIDIsSet() && order_->getOrderID() != "") {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(400, "Must specify only one of orderID or origClOrdId", content);
    }
    if (order_->origClOrdIDIsSet() && !order_->clOrdIDIsSet()){
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(400, "Must specify ClOrdID if OrigClOrdID is set", content);
    }
    if ((int)order_->getOrderQty() % std::stoi(_config->get("lotSize", "100")) != 0) {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(400, "Orderty is not a multiple of lotsize", content);
    }
    auto total_cost = 1/(order_->getOrderQty() * order_->getPrice());
    if (total_cost > _margin->getWalletBalance()) {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        std::stringstream reason;
        reason << "Account has insufficient Available Balance, " << total_cost << " XBt required";
        throw api::ApiException(400, reason.str(), content);
    }
    return true;
}

void TestOrdersApi::init(std::shared_ptr<Config> config_) {
    _config = config_;
    _leverage = std::atof(_config->get("leverage", "10").c_str());
    _leverageType = _config->get("leverageType", "ISOLATED");
}

const std::shared_ptr<MarginCalculator>& TestOrdersApi::getMarginCalculator() const {
    return _marginCalculator;
}

void TestOrdersApi::setMarginCalculator(const std::shared_ptr<MarginCalculator>& marginCalculator) {
    _marginCalculator = marginCalculator;
}
