//
// Created by Rory McStay on 18/06/2021.
//
#include "TestOrdersApi.h"
#include "ApiException.h"

#include <boost/none.hpp>
#include <memory>
#include <stdexcept>
#include <utility>

#define _TURN_OFF_PLATFORM_STRING
#include <Object.h>
#include <Allocation.h>
#include <model/Margin.h>

#include "Utils.h"
#include "Functional.h"


TestOrdersApi::TestOrdersApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr)
:   _orders()
,   _newOrders()
,   _rejects()
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
    order->setClOrdID(clOrdID.value());
    order->setOrigClOrdID(origClOrdID.value());
    auto origOrder = checkOrderExists(order);
    double new_price;
    double new_qty;
    order->setOrdStatus("PendingReplace");
    order->setLeavesQty(leavesQty.value());
    if (price.has_value()) {
        order->setPrice(price.value());
    }
    if (orderID.has_value())
        order->setOrderID(orderID.value());
    if (orderQty.has_value()) {
        new_qty = orderQty.value();
        order->setOrderQty(orderQty.value());
    }
    if (stopPx.has_value()) 
        order->setStopPx(stopPx.value());
    if (pegOffsetValue.has_value())
        order->setPegOffsetValue(pegOffsetValue.value());
    if (text.has_value())
        order->setText(text.value());

    auto origQty = origOrder->getOrderQty();
    double diff = order->getLeavesQty() - origOrder->getLeavesQty();
    checkValidAmend(order, origOrder);
    amend_order(order, origOrder);
    set_order_timestamp(order);
    order->setOrdStatus("Replaced");
    // populate for data purposes
    order->setOrderQty(origQty + diff);
    order->setOrdType(origOrder->getOrdType());
    order->setCumQty(origOrder->getCumQty());
    order->setExecInst(origOrder->getExecInst());
    order->setPrice(origOrder->getPrice());
    order->setSymbol(origOrder->getSymbol());
    order->setSide(origOrder->getSide());
    order->setTimeInForce(origOrder->getTimeInForce());
    order->setAvgPx(order->getAvgPx());
    _orderAmends.push(order);
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
        auto& origOrder = _orders[clOrdID.value()];
        origOrder->setOrdStatus("Canceled");
        auto event_order = std::make_shared<model::Order>();
        auto event_json = _orders[clOrdID.value()]->toJson();
        event_order->fromJson(event_json);
        origOrder->setOrderQty(0.0);
        origOrder->setLeavesQty(0.0);
        set_order_timestamp(origOrder);
        ordersRet.push_back(origOrder);
        _orderCancels.push(event_order);
        _allEvents.push(event_order);
        _orders.erase(clOrdID.value());
    }
    return pplx::task_from_result(ordersRet);
}

pplx::task<std::vector<std::shared_ptr<model::Order>>>
TestOrdersApi::order_cancelAll(boost::optional<utility::string_t> symbol, boost::optional<utility::string_t> filter,
                               boost::optional<utility::string_t> text) {
    std::vector<std::shared_ptr<model::Order>> out = {};
    for (auto& orders : _orders) {
        order_cancel(boost::none, orders.second->getClOrdID(), boost::none);
        out.push_back(orders.second);
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
    order->setOrderQty(orderQty.value());
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
    ASSERT_EQ(val1, val2) << " The values " << LOG_NVP(#val1, val1) << " and " << LOG_NVP(#val2, val2) << " "

#define PVAR(order, name_)  #name_ "="  << (order)->get##name_() << " "
void TestOrdersApi::operator >> (const std::string &outEvent_) {
    auto eventType = getEventTypeFromString(outEvent_);

    std::stringstream failMessage;
    failMessage << "Event filter:\n\t" << outEvent_ << "\nnot satisifed. Reason: ";
    if (eventType == "NONE") {
        if (!_newOrders.empty() || !_orderCancels.empty() || !_orderAmends.empty()) {
            failMessage << "There are events still pending!\n";
            while (!_newOrders.empty()) {
                auto order = _newOrders.front();
                if (!order) {
                    failMessage << "NULL";
                    break;
                }
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
                if (!order) {
                    failMessage << "NULL";
                    break;
                }
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
                if (!order) {
                    failMessage << "NULL";
                    break;
                }
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
            throw std::runtime_error(failMessage.str());
        }
        return;
    }
    auto params = Params(outEvent_);
    auto expectedOrder = fromJson<model::Order>(params.asJson());
    if (eventType == "ORDER_NEW") {
        if (_newOrders.empty()) {
            failMessage << "No new orders made\n";
            throw std::runtime_error(failMessage.str());
        }
        auto latestOrder = _newOrders.front();
        CHECK_VAL(latestOrder->getPrice(), expectedOrder->getPrice()) << failMessage.str();
        CHECK_VAL(latestOrder->getSide(), expectedOrder->getSide()) << failMessage.str();
        CHECK_VAL(latestOrder->getOrderQty(), expectedOrder->getOrderQty()) << failMessage.str();
        CHECK_VAL(latestOrder->getLeavesQty(), expectedOrder->getLeavesQty()) << failMessage.str();
        CHECK_VAL(latestOrder->getSymbol(), expectedOrder->getSymbol()) << failMessage.str();
    } else if (eventType == "ORDER_AMEND") {
        checkOrderExists(expectedOrder);
        if (_orderAmends.empty()) {
            failMessage << "No order amends made\n";
            throw std::runtime_error(failMessage.str());
        }
        auto orderAmend = _orderAmends.front();
        if (orderAmend->priceIsSet()) {
            CHECK_VAL(orderAmend->getPrice(), expectedOrder->getPrice()) << failMessage.str();
        } else if (orderAmend->leavesQtyIsSet()) {
            CHECK_VAL(orderAmend->getLeavesQty(), expectedOrder->getLeavesQty()) << failMessage.str();
        } else {
            throw std::runtime_error("Only Price or Qty amend supported at this time.");
        }
        CHECK_VAL(orderAmend->getOrderID(), expectedOrder->getOrderID()) << failMessage.str();

    } else if (eventType == "ORDER_CANCEL") {
        if (_orderCancels.empty()) {
            failMessage << "No cancels made\n";
            throw std::runtime_error(failMessage.str());
        }
        auto orderCancel = _orderCancels.front();
        CHECK_VAL(orderCancel->getSymbol(), expectedOrder->getSymbol()) << failMessage.str();
        CHECK_VAL(orderCancel->getOrderID(), expectedOrder->getOrderID()) << failMessage.str();
        CHECK_VAL(orderCancel->getClOrdID(), expectedOrder->getClOrdID()) << failMessage.str();
    } else {
        failMessage << R"(Must specify event type - One of "ORDER_NEW", "ORDER_AMEND", "ORDER_CANCEL")";
        std::runtime_error(failMessage.str());

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
    auto event_order = std::make_shared<model::Order>();
    auto event_json = order_->toJson();
    event_order->fromJson(event_json);
    _newOrders.push(event_order);
    _allEvents.push(event_order);
    
}


void TestOrdersApi::amend_order(const std::shared_ptr<model::Order>& amendRequest_,
                                const std::shared_ptr<model::Order>& originalOrder_
                                ) {
    originalOrder_->setOrdStatus(originalOrder_->getCumQty() > 0.0 ? "PartiallyFilled" : "New");
    originalOrder_->setClOrdID(amendRequest_->getClOrdID());
    originalOrder_->setOrigClOrdID(amendRequest_->getOrigClOrdID());
    amendRequest_->setOrderID(originalOrder_->getOrderID());
    if (amendRequest_->priceIsSet()) {
        originalOrder_->setPrice(amendRequest_->getPrice());
    } else if (amendRequest_->leavesQtyIsSet()) {
        auto qtyChange = amendRequest_->getLeavesQty() - originalOrder_->getLeavesQty();
        originalOrder_->setOrderQty(originalOrder_->getOrderQty() + qtyChange);
    } else {
        throw std::runtime_error("Only price and quantity amends supported currently.");
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
                    << LOG_NVP("LastQty", exec_->getLastQty())
                    << LOG_NVP("LastPx", exec_->getLastPx())
                    << AixLog::Color::none);

    assert(exec_->lastPxIsSet() && exec_->lastQtyIsSet()
           && "LastPx and LastQty must be specified for executions");

    {   // update wallet balance
        auto lastQty = exec_->getLastQty();
        auto lastPx = exec_->getLastPx();
        auto cost = func::get_cost(lastPx, lastQty, _position->getLeverage());
        double newBalance;
        if (not almost_equal(_position->getCurrentQty(), 0.0) and sgn(lastQty) == sgn(_position->getCurrentQty())) {
            // balance must be reduced since the execution is extending our position
            newBalance = _margin->getWalletBalance() - cost;
        }
        else { // pay less than the cost, since remainder from our position
            if (std::abs(_position->getCurrentQty()) > std::abs(lastQty)) {
                // paying entirely with position, so we gain the cost
                newBalance = _margin->getWalletBalance() + cost;
            } else {
                // pay partially with positon
                cost = func::get_cost(lastPx, _position->getCurrentQty() - lastQty, _position->getLeverage());
                newBalance = _margin->getWalletBalance() - cost;
            }
        }
        _margin->setWalletBalance(newBalance);
    }
    { // update the order
        auto order = _orders.at(exec_->getClOrdID());
        order->setCumQty(order->getCumQty() + exec_->getLastQty());
        order->setLeavesQty(order->getLeavesQty() - exec_->getLastQty());
        auto newQty = order->getCumQty();
        auto oldQty = newQty - exec_->getLastQty();
        auto newAvgPx = (order->getAvgPx() * oldQty/newQty) + (exec_->getLastQty() * exec_->getLastQty()/newQty);
        order->setAvgPx(newAvgPx);
    }
    { // update costs and quantity
        price_t currentCost = _position->getCurrentCost();
        qty_t currentSize = _position->getCurrentQty();
        price_t costDelta = func::get_cost(exec_->getLastPx(), exec_->getLastQty(), _position->getLeverage());
        qty_t positionDelta = (exec_->getSide() == "Buy")
                                  ? exec_->getLastQty()
                                  : -exec_->getLastQty();
        qty_t newPosition = currentSize + positionDelta;
        price_t newCost = currentCost + costDelta;
        _position->setCurrentQty(newPosition);
        _position->setCurrentCost(newCost);
    }
    { // set the breakeven price
        auto bkevenPrice = _position->getCurrentQty()/_position->getCurrentCost();
        _position->setBreakEvenPrice(bkevenPrice);
    }
    { // unrealised pnl calculation
        auto unrealisedPnl = _marginCalculator->getUnrealisedPnL(_position);
        _position->setUnrealisedPnl(unrealisedPnl);
    }
    { // update the margin
        auto marginBalance = _margin->getWalletBalance() + _position->getUnrealisedPnl();
        _margin->setMarginBalance(marginBalance);
        double maintenanceMargin = _marginCalculator->getMarginAmount(_position);
        _position->setMaintMarginReq(maintenanceMargin);
        _margin->setMaintMargin(maintenanceMargin);
        _margin->setAvailableMargin(_margin->getWalletBalance() - maintenanceMargin);
    }
    { // liquidation price
        auto leverageType = "ISOLATED";
        auto liqPrice = _marginCalculator->getLiquidationPrice(
                _position->getLeverage(),
                leverageType,
                _marginCalculator->getMarkPrice(),
                _position->getCurrentQty(),
                _margin->getWalletBalance()
        );
        _position->setLiquidationPrice(liqPrice);
        _position->setExecQty(exec_->getLastQty());
    }
    // timestamp
    _position->setTimestamp(exec_->getTimestamp());

    LOGINFO(AixLog::Color::GREEN
                    << "New Position: "
                    << LOG_NVP("CurrentCost", _position->getCurrentCost())
                    << LOG_NVP("Size", _position->getCurrentQty())
                    << LOG_NVP("Time", _position->getTimestamp().to_string())
                    << AixLog::Color::none);
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
    }
    for (auto ord_kvp : _orders) {
        if (ord_kvp.second->getOrigClOrdID() == order->getClOrdID()) {
            return ord_kvp.second;
        }
    }
    // reject amend request
    std::stringstream str;
    auto content = std::make_shared<std::istringstream>(order->toJson().serialize());
    str << "Original order " << LOG_NVP("OrderID",order->getOrderID())
        << LOG_NVP("ClOrdID", order->getClOrdID())
        << LOG_NVP("OrigClOrdID", order->getOrigClOrdID()) << " not found.";
    throw api::ApiException(404, str.str(), content);
}


/*
 * Check order is valid given current status of margin and that parameters are correct.
 */
bool TestOrdersApi::validateOrder(const std::shared_ptr<model::Order> &order_) {
    std::string error;
    bool fail = false;
    if (order_->origClOrdIDIsSet() && order_->getOrderID() != "") {
        error = "Must specify only one of orderID or origClOrdId";
        fail = true;
    }
    if (order_->origClOrdIDIsSet() && !order_->clOrdIDIsSet()) {
        error = "Must specify ClOrdID if OrigClOrdID is set";
        fail = true;
    }
    if ((int)order_->getOrderQty() % std::stoi(_config->get("lotSize", "100")) != 0) {
        error = "Orderty is not a multiple of lotsize";
        fail = true;
    }
    auto total_cost = 1/(order_->getOrderQty() * order_->getPrice());
    if (total_cost > _margin->getWalletBalance()) {
        std::stringstream reason;
        reason << "Account has insufficient Available Balance, " << total_cost << " XBt required";
        error = reason.str();
        fail = true;
    }
    if (fail) {
        auto msg = order_->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        order_->setText(error);
        order_->setOrdStatus("Rejected");
        _rejects.push(order_);
        throw api::ApiException(400, error, content);
    }
    return true;
}


bool TestOrdersApi::checkValidAmend(std::shared_ptr<model::Order> requestedAmend,
                                    std::shared_ptr<model::Order> originalOrder) {

    if (requestedAmend->leavesQtyIsSet() and requestedAmend->orderQtyIsSet()) {
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        requestedAmend->setText("Cannot specify both OrderQty and LeavesQty");
        requestedAmend->setOrdStatus(originalOrder->getOrdStatus());
        _rejects.push(requestedAmend);
        throw api::ApiException(400, requestedAmend->getText(), content);
    }
    if ((requestedAmend->leavesQtyIsSet() || requestedAmend->orderQtyIsSet())
         && requestedAmend->priceIsSet()) {
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        requestedAmend->setText("Cannot specify both Qty and Price amend");
        requestedAmend->setOrdStatus(originalOrder->getOrdStatus());
        _rejects.push(requestedAmend);
        throw api::ApiException(400, requestedAmend->getText(),content);
    }
    if (requestedAmend->orderQtyIsSet()) {
        throw std::runtime_error("Order amends via OrderQty not implemented.");
    }
    if (requestedAmend->leavesQtyIsSet() &&
        requestedAmend->getLeavesQty() > originalOrder->getLeavesQty()) {
        auto additionalCost = 1/originalOrder->getPrice()* (requestedAmend->getLeavesQty() - originalOrder->getLeavesQty());
        if (additionalCost > _margin->getWalletBalance()) {
            auto msg = requestedAmend->toJson().serialize();
            auto content = std::make_shared<std::istringstream>(msg);
            requestedAmend->setText("Insufficient funds available for amend. additionalCost="
                    + std::to_string(additionalCost)
                    + ", balance=" + std::to_string(_margin->getWalletBalance()));
            requestedAmend->setOrdStatus(originalOrder->getOrdStatus());
            _rejects.push(requestedAmend);
            throw api::ApiException(400, requestedAmend->getText(), content);
        }
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
std::shared_ptr<model::Order>
TestOrdersApi::getEvent(const std::string& event_) {
    std::shared_ptr<model::Order> order;
    if (event_ == "NONE") {
        return nullptr;
    } else if (event_ == "ORDER_NEW") {
        order = _newOrders.front();
        _newOrders.pop();
    } else if (event_ == "ORDER_AMEND") {
        order = _orderAmends.front();
        _orderAmends.pop();
    } else if (event_ == "ORDER_CANCEL") {
        order = _orderCancels.front();
        _orderCancels.pop();
    } else {
        throw std::runtime_error("Unrecognized event type " + event_);
    }
    return order;
}
