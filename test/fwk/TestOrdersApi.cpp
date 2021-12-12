//
// Created by Rory McStay on 18/06/2021.
//
#include "TestOrdersApi.h"

#include <stdexcept>
#include <utility>

#define _TURN_OFF_PLATFORM_STRING
#include <Object.h>
#include <Allocation.h>
#include <model/Margin.h>

#include "Utils.h"


TestOrdersApi::TestOrdersApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr)
:   _orders()
,   _newOrders()
,   _orderAmends()
,   _orderCancels()
,   _oidSeed(0) {

    LOGINFO("TestOrdersApi initialised!");

}

pplx::task<std::shared_ptr<model::Order>>
TestOrdersApi::order_amend(boost::optional<utility::string_t> orderID,
                           boost::optional<utility::string_t> origClOrdID,
                           boost::optional<utility::string_t> clOrdID,
                           boost::optional<double> simpleOrderQty,
                           boost::optional<double> orderQty,
                           boost::optional<double> simpleLeavesQty,
                           boost::optional<double> leavesQty,
                           boost::optional<double> price,
                           boost::optional<double> stopPx,
                           boost::optional<double> pegOffsetValue,
                           boost::optional<utility::string_t> text) {
    auto order = std::make_shared<model::Order>();
    order->setOrdStatus("PendingNew");
    order->setClOrdID(clOrdID.value());
    order->setOrigClOrdID(origClOrdID.value());
    order->setLeavesQty(orderQty.value());
    if (orderID.has_value())
        order->setOrderID(orderID.value());
    if (price.has_value())
        order->setPrice(price.value());
    if (stopPx.has_value())
        order->setStopPx(stopPx.value());
    if (pegOffsetValue.has_value())
        order->setPegOffsetValue(pegOffsetValue.value());
    if (text.has_value())
        order->setText(text.value());
    auto origOrder = checkOrderExists(order);
    checkValidAmend(order, origOrder);
    amend_order(order, origOrder);
    set_order_timestamp(order);
    order->setOrdStatus("Replaced");
    _orderAmends.emplace(order);
    _allEvents.push(order);
    return pplx::task_from_result(order);
}


pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancel(boost::optional<utility::string_t> orderID,
                            boost::optional<utility::string_t> clOrdID,
                            boost::optional<utility::string_t> text) {
    std::vector<std::shared_ptr<model::Order>> ordersRet;

    if (_orders.find(clOrdID.value()) == _orders.end()) {
        LOGWARN(LOG_NVP("ClOrdID",clOrdID.value()) << " not found to cancel.");
        std::stringstream str;
        auto content = std::make_shared<std::istringstream>(R"({"clOrdID": ")" + clOrdID.value() +  R"(" })");
        throw api::ApiException(404, "Order not found to cancel", content);
    } else {
        _orders[clOrdID.value()]->setOrdStatus("Canceled");
        _orders[clOrdID.value()]->setOrderQty(0.0);
        _orders[clOrdID.value()]->setLeavesQty(0.0);
        set_order_timestamp(_orders[clOrdID.value()]);
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
TestOrdersApi::order_new(utility::string_t symbol,
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
                         boost::optional<utility::string_t> text) {
    auto order = std::make_shared<model::Order>();
    order->setOrdStatus("PendingNew");
    order->setSymbol(std::move(symbol));
    order->setSide(side.value());

    order->setClOrdID(clOrdID.value());
    order->setLeavesQty(orderQty.value());
    order->setPrice(price.value());
    order->setOrdType(ordType.value());
    order->setTimeInForce(timeInForce.value());
    if (displayQty.has_value())
        order->setDisplayQty(displayQty.value());
    if (stopPx.has_value())
        order->setStopPx(stopPx.value());
    if (clOrdLinkID.has_value())
        order->setClOrdLinkID(clOrdLinkID.value());
    if (pegOffsetValue.has_value())
        order->setPegOffsetValue(pegOffsetValue.value());
    if (pegPriceType.has_value())
        order->setPegPriceType(pegPriceType.value());
    if (execInst.has_value())
        order->setExecInst(execInst.value());
    if (contingencyType.has_value())
        order->setContingencyType(contingencyType.value());
    if (text.has_value())
        order->setText(text.value());
    validateOrder(order);
    add_order(order);
    return pplx::task_from_result(order);
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
    failMessage << R"(Must specify event type - One of "ORDER_NEW", "ORDER_AMEND", "ORDER_CANCEL")";
    throw std::runtime_error(failMessage.str());
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


void TestOrdersApi::amend_order(const std::shared_ptr<model::Order>& amendRequest_,
                                const std::shared_ptr<model::Order>& originalOrder_
                                ) {
    originalOrder_->setOrdStatus(originalOrder_->getCumQty() > 0.0 ? "PartiallyFilled" : "New");
    originalOrder_->setClOrdID(amendRequest_->getClOrdID());
    originalOrder_->setOrigClOrdID(amendRequest_->getOrigClOrdID());
    if (amendRequest_->priceIsSet()) {
        originalOrder_->setPrice(amendRequest_->getPrice());
    }
    if (amendRequest_->leavesQtyIsSet()) {
        auto qtyChange = amendRequest_->getLeavesQty() - originalOrder_->getLeavesQty();
        originalOrder_->setOrderQty(originalOrder_->getOrderQty() + qtyChange);
    }
    _orders.erase(originalOrder_->getOrigClOrdID());
    _orders[originalOrder_->getClOrdID()] = originalOrder_;
    _orderAmends.push(amendRequest_);
    _allEvents.push(amendRequest_);
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

std::shared_ptr<model::Order> TestOrdersApi::checkOrderExists(const std::shared_ptr<model::Order> &order) {
    // hack storing orders under both id's.
    if (_orders.find(order->getClOrdID()) != _orders.end()
        || _orders.find(order->getOrigClOrdID()) != _orders.end()) {
        // is amend
        return _orders.find(order->getClOrdID()) != _orders.end() ? _orders[order->getClOrdID()] : _orders[order->getOrigClOrdID()];
    } else {
        // reject amend request, fail test
        std::stringstream str;
        auto content = std::make_shared<std::istringstream>(order->toJson().serialize());
        str << "Original order " << LOG_NVP("OrderID",order->getOrderID())
            << LOG_NVP("ClOrdID", order->getClOrdID())
            << LOG_NVP("OrigClOrdID", order->getOrigClOrdID()) << " not found.";
        throw api::ApiException(404, str.str(), content);
    }

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


bool TestOrdersApi::checkValidAmend(std::shared_ptr<model::Order> requestedAmend,
                                    std::shared_ptr<model::Order> originalOrder) {

    if (requestedAmend->leavesQtyIsSet() and requestedAmend->orderQtyIsSet()) {
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(
            400, "Cannot specify both OrderQty and LeavesQty", content);
    }
    if ((requestedAmend->leavesQtyIsSet() || requestedAmend->orderQtyIsSet())
         && requestedAmend->priceIsSet()) {
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(
            400, "Cannot specify both Qty and Price amend",
            content);
    }
    if (requestedAmend->orderQtyIsSet()) {
        throw std::runtime_error("Order amends via OrderQty not implemented.");
    }
    if (requestedAmend->leavesQtyIsSet() &&
        requestedAmend->getLeavesQty() > originalOrder->getLeavesQty()) {
        auto additionalCost = 1/(requestedAmend->getLeavesQty() - originalOrder->getLeavesQty()) * originalOrder->getPrice();
        if (additionalCost > _margin->getWalletBalance()) {
            auto msg = requestedAmend->toJson().serialize();
            auto content = std::make_shared<std::istringstream>(msg);
            throw api::ApiException(
                400, "Insufficient funds available for amend.",
                content);
        }
    } else if (requestedAmend->leavesQtyIsSet() &&
               almost_equal(originalOrder->getLeavesQty(), 0.0)) {
        // too late
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        throw api::ApiException(
            400, "Insufficient funds available for amend.",
            content);

    } else if (requestedAmend->priceIsSet()) {
    } else {
        throw std::runtime_error("Couldn't determine amend action.");
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