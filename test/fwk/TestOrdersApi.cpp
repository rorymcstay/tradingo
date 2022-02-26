//
// Created by Rory McStay on 18/06/2021.
//
#include <boost/none.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#define _TURN_OFF_PLATFORM_STRING
#include "TestOrdersApi.h"
#include "ApiException.h"
#include "model/Execution.h"
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

void TestOrdersApi::setMarketData(const std::shared_ptr<TestMarketData>& marketData_) {
    _marketData = marketData_;
}

/// look up orders for old client order id.
std::shared_ptr<model::Order>
get_order_for_amend(
        const std::string& origClOrdID_,
        const std::map<std::string, std::shared_ptr<model::Order>>& orders_) {

    for (auto& ord_kvp : orders_) {
        if (ord_kvp.second->getOrigClOrdID() == origClOrdID_) {
            return ord_kvp.second;
        }
    }
    return nullptr;
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
    std::stringstream str;
    if (!origOrder) {
        _rejects.push(order);
        std::stringstream str;
        auto content = std::make_shared<std::istringstream>(order->toJson().serialize());
        str << "Original order " << LOG_NVP("OrderID",order->getOrderID())
            << LOG_NVP("ClOrdID", order->getClOrdID())
            << LOG_NVP("OrigClOrdID", order->getOrigClOrdID()) << " not found.";
        throw api::ApiException(404, str.str(), content);
    }
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
    // rejects if invalid
    checkValidAmend(order, origOrder);

    // Do the replace
    origOrder->setOrdStatus(origOrder->getCumQty() > 0.0 ? "PartiallyFilled" : "New");
    origOrder->setClOrdID(order->getClOrdID());
    origOrder->setOrigClOrdID(order->getOrigClOrdID());
    order->setOrderID(origOrder->getOrderID());
    // negative in the case of amend down, therefore increase in balance.
    auto qty_delta = order->getLeavesQty() - origOrder->getLeavesQty();
    auto total_cost = func::get_additional_cost(getPosition(origOrder->getSymbol()), qty_delta, order->getPrice());
    // in the case of amend up, and long position, we want this to reduce
    getMargin()->setWalletBalance(getMargin()->getWalletBalance() - total_cost);
    if (order->priceIsSet()) {
        origOrder->setPrice(order->getPrice());
    } else if (order->leavesQtyIsSet()) {
        auto qtyChange = order->getLeavesQty() - origOrder->getLeavesQty();
        origOrder->setOrderQty(order->getOrderQty() + qtyChange);
    } else {
        throw std::runtime_error("Only price and quantity amends supported currently.");
    }
    _orders.erase(origOrder->getOrigClOrdID());
    _orders[origOrder->getClOrdID()] = origOrder;
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
    
    // pattern all api calls to do this last
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
        auto reject = std::make_shared<model::Order>();
        reject->setText("Order not found to cancel");
        reject->setOrderID(clOrdID.value());
        _rejects.push(reject);
        throw api::ApiException(404, reject->getText(), content);
    }
    auto& origOrder = _orders.at(clOrdID.value());
    origOrder->setOrdStatus("Canceled");
    auto event_order = std::make_shared<model::Order>();
    auto event_json = _orders[clOrdID.value()]->toJson();
    event_order->fromJson(event_json);
    // update balance after cancel
    auto total_cost = func::get_cost(origOrder->getPrice(), origOrder->getOrderQty(), getPosition(origOrder->getSymbol())->getLeverage());
    getMargin()->setWalletBalance(getMargin()->getWalletBalance() + total_cost);
    origOrder->setOrderQty(0.0);
    origOrder->setLeavesQty(0.0);
    set_order_timestamp(origOrder);
    ordersRet.push_back(origOrder);

    _orderCancels.push(event_order);
    _allEvents.push(event_order);
    _orders.erase(clOrdID.value());
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
    // rejects if invalid
    validateOrder(order);
    // Add the order
    _oidSeed++;
    order->setOrderID(std::to_string(_oidSeed));
    _orders[order->getClOrdID()] = order;
    order->setOrdStatus("New");
    order->setLeavesQty(order->getOrderQty());
    order->setCumQty(0.0);
    set_order_timestamp(order);
    auto event_order = std::make_shared<model::Order>();
    auto event_json = order->toJson();
    event_order->fromJson(event_json);
    _newOrders.push(event_order);
    _allEvents.push(event_order);
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
        std::string expected_order_id = std::to_string(params.asJson()["orderID"].as_integer());
        if (_orderAmends.empty()) {
            failMessage << "No order amends made\n";
            throw std::runtime_error(failMessage.str());
        }
        if (not (get_order_for_amend(expectedOrder->getClOrdID(), _orders) 
                    || _orders.find(expectedOrder->getClOrdID()) != _orders.end())) {
            // is amend
            failMessage << " The order " 
                << LOG_NVP("OrderID", expected_order_id) 
                << LOG_NVP("ClOrdID", expectedOrder->getClOrdID())
                << " does not exist in the test\n";
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
        CHECK_VAL(orderAmend->getOrderID(), expected_order_id) << failMessage.str();

    } else if (eventType == "ORDER_CANCEL") {
        std::string expected_order_id = std::to_string(params.asJson()["orderID"].as_integer());
        if (_orderCancels.empty()) {
            failMessage << "No cancels made\n";
            throw std::runtime_error(failMessage.str());
        }
        auto orderCancel = _orderCancels.front();
        CHECK_VAL(orderCancel->getSymbol(), expectedOrder->getSymbol()) << failMessage.str();
        CHECK_VAL(orderCancel->getOrderID(), expected_order_id) << failMessage.str();
        CHECK_VAL(orderCancel->getClOrdID(), expectedOrder->getClOrdID()) << failMessage.str();
    } else {
        failMessage << R"(Must specify event type - One of "ORDER_NEW", "ORDER_AMEND", "ORDER_CANCEL")";
        std::runtime_error(failMessage.str());

    }
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


void TestOrdersApi::onExecution(const std::shared_ptr<model::Execution>& exec_) {

    auto& order = orders().at(exec_->getClOrdID());
    qty_t qtyDelta = (exec_->getSide() == "Buy") ? exec_->getLastQty() : - exec_->getLastQty();
    order->setCumQty(order->getCumQty() + qtyDelta);
    order->setLeavesQty(order->getLeavesQty() - qtyDelta);
    order->setOrdStatus(exec_->getOrdStatus());
    auto newCumQty = order->getCumQty() + qtyDelta;
    auto oldCumQty = order->getCumQty() - qtyDelta;
    auto newAvgPx = (order->getAvgPx() * oldCumQty/newCumQty) + (exec_->getLastPx() * exec_->getLastQty()/newCumQty);
    order->setAvgPx(newAvgPx);
    order->setTimestamp(exec_->getTimestamp());

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


const std::shared_ptr<model::Position> &TestOrdersApi::getPosition(const std::string& symbol_) const {
    return _marketData->getPositions().at(symbol_);
}


const std::shared_ptr<model::Margin>& TestOrdersApi::getMargin() const {
    return _marketData->getMargin();
}


std::shared_ptr<model::Order> TestOrdersApi::checkOrderExists(const std::shared_ptr<model::Order> &order) {
    if (_orders.find(order->getClOrdID()) != _orders.end()
        || _orders.find(order->getOrigClOrdID()) != _orders.end()) {
        return _orders.find(order->getClOrdID()) != _orders.end() ? _orders[order->getClOrdID()] : _orders[order->getOrigClOrdID()];
    }
    // is amend
    auto found = get_order_for_amend(order->getOrderID(), _orders);
    if (found) {
        return found;
    }
    return nullptr;
}


/*
 * Check order is valid given current status of margin and that parameters are correct.
 */
bool TestOrdersApi::validateOrder(const std::shared_ptr<model::Order> &order_) {
    std::string error;
    auto& instrument = _marketData->getInstruments().at(order_->getSymbol());
    auto& position = getPosition(order_->getSymbol());
    bool fail = false;
    if (order_->origClOrdIDIsSet() && order_->getOrderID() != "") {
        error = "Must specify only one of orderID or origClOrdId";
        fail = true;
    }
    if (order_->origClOrdIDIsSet() && !order_->clOrdIDIsSet()) {
        error = "Must specify ClOrdID if OrigClOrdID is set";
        fail = true;
    }
    if ((int)order_->getOrderQty() % (int)instrument->getLotSize() != 0.0) {
        error = "Orderty is not a multiple of lotsize";
        fail = true;
    }
    auto total_cost = func::get_cost(order_->getPrice(), order_->getOrderQty(), position->getLeverage());
    if (total_cost > getMargin()->getWalletBalance()) {
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
    // negative incase of amend down.
    auto quantity_delta = requestedAmend->getLeavesQty() - originalOrder->getLeavesQty();
    auto additionalCost = func::get_cost(originalOrder->getPrice(), quantity_delta, getPosition(originalOrder->getSymbol())->getLeverage());
    // if amending down, always false
    if (additionalCost > getMargin()->getWalletBalance()) {
        auto msg = requestedAmend->toJson().serialize();
        auto content = std::make_shared<std::istringstream>(msg);
        requestedAmend->setText("Insufficient funds available for amend. additionalCost="
                + std::to_string(additionalCost)
                + ", balance=" + std::to_string(getMargin()->getWalletBalance()));
        requestedAmend->setOrdStatus(originalOrder->getOrdStatus());
        _rejects.push(requestedAmend);
        throw api::ApiException(400, requestedAmend->getText(), content);
    }

    return true;
}


void TestOrdersApi::init(std::shared_ptr<Config> config_) {
    _config = config_;
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
