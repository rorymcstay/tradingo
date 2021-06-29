//
// Created by rory on 14/06/2021.
//


#include "MarketData.h"
#include "Utils.h"
#include <model/Position.h>
#include <model/Order.h>




std::string MarketData::getConnectionString() {
    // TODO handle auth
    return _connectionString + "/realtime?subscribe=trade:" + _symbol + ",instrument:"+_symbol + ",quote:"+_symbol;
}

std::shared_ptr<Event> MarketDataInterface::read() {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto event= _eventBuffer.empty() ? nullptr : _eventBuffer.top();
    if (event) {
        INFO(LOG_NVP("Queue Size", _eventBuffer.size()));
        _eventBuffer.pop();
    }
    return event;
}

MarketData::~MarketData() {
    _wsClient->close();
}

MarketData::MarketData(std::string connectionString_, std::string symbol_)
:   MarketDataInterface()
,   _connectionString(std::move(connectionString_))
,   _symbol(std::move(symbol_))
,   _wsClient(std::make_shared<ws::client::websocket_callback_client>())
,   _initialised(false)
{
    _wsClient->set_message_handler(
            [&](const ws::client::websocket_incoming_message& in_msg) {
                auto msg = in_msg.extract_string();
                auto stringVal = msg.get();
                //std::cout << stringVal << std::endl;
                web::json::value msgJson = web::json::value::parse(stringVal);
                if (msgJson.has_field("info")) {
                    INFO("info: " << msgJson["info"].as_string());
                    INFO("response" << msgJson.serialize());
                    return;
                }
                if (msgJson.has_field("success") && msgJson.at("success").as_bool()) {
                    _initialised = true;
                    INFO("Initialised " << msgJson.serialize());
                    return;
                }

                const std::string& table = msgJson.at("table").as_string();
                const std::string& action = msgJson.at("action").as_string();
                web::json::array& data = msgJson.at("data").as_array();

                if (table == "quote") {
                    auto quotes = getData<model::Quote, decltype(_quotePool)>(data, _quotePool);
                    handleQuotes(quotes, action);
                } else if (table == "trade") {
                    auto trades = getData<model::Trade, decltype(_tradePool)>(data, _tradePool);
                    handleTrades(trades, action);
                } else if (table == "executions") {
                    auto exec = getData<model::Execution, decltype(_execPool)>(data, _execPool);
                    handleExecutions(exec, action);
                } else if (table == "positions") {
                    auto positions = getData<model::Position, decltype(_positionPool)>(data, _positionPool);
                    handlePositions(positions, action);
                } else if (table == "orders") {
                    auto orders = getData<model::Order, decltype(_orderPool)>(data,_orderPool);
                    handleOrders(orders, action);
                }
            });

    _wsClient->set_close_handler(
            [](ws::client::websocket_close_status stat, const std::string& message_, std::error_code err_) { std::cout << message_ << '\n';}
    );
    std::string conString = getConnectionString();
    _wsClient->connect(conString);

    // sleep until connected.
    while (not _initialised)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

template<typename T>
void MarketDataInterface::update(const std::vector<std::shared_ptr<T>> &data_) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    for (const auto& row : data_) {
        _eventBuffer.push(std::make_shared<Event>(row));
    }

}

void MarketDataInterface::updatePositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {
    insertPositions(positions_);
}

void MarketDataInterface::insertPositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {

    for (auto& pos : positions_)
    {
        _positions[getPositionKey(pos)] = pos;
    }
}

void MarketDataInterface::removePositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {

    for (auto& pos : positions_)
        _positions.erase(getPositionKey(pos));
}

void MarketDataInterface::handleQuotes(const std::vector<std::shared_ptr<model::Quote>>& quotes_, const std::string &action_) {
    update(quotes_);
}

void MarketDataInterface::handleTrades(std::vector<std::shared_ptr<model::Trade>>& trades_, const std::string &action_) {
    update(trades_);
}

void MarketDataInterface::updateKey(const web::json::array& values_, std::vector<std::string>& key_) {
    key_.clear();
    for (auto& val : values_) {
        key_.push_back(val.as_string());
    }
}

void MarketDataInterface::handlePositions(std::vector<std::shared_ptr<model::Position>> &positions_,
                                          const std::string &action_) {

    if (action_ == "update") {
        updatePositions(positions_);
    } else if (action_ == "insert" or action_ == "partial") {
        insertPositions(positions_);
    } else if (action_ == "delete") {
        removePositions(positions_);
    }
}

void MarketDataInterface::handleExecutions(std::vector<std::shared_ptr<model::Execution>> &execs_,
                                           const std::string &action_) {
    if (action_ == "update" or action_ == "insert" or action_ == "partial") {
        for (auto& ex : execs_)
            _executions.push(ex);
    }
}

void MarketDataInterface::handleOrders(std::vector<std::shared_ptr<model::Order>>& orders_, const  std::string &action_) {

    if (action_ == "partial" or action_ == "insert") {
        insertOrders(orders_);
    } else if (action_ == "update") {
        updateOrders(orders_);
    } else if (action_ == "delete") {
        removeOrders(orders_);
    }

}

void MarketDataInterface::removeOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_)
        _orders.erase(getOrderKey(ord));
}

void MarketDataInterface::insertOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_) {
        _orders.insert(std::pair(getOrderKey(ord), ord));
    }
}

void MarketDataInterface::updateOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_)
        _orders[getOrderKey(ord)] = ord;
}

std::string getPositionKey(const std::shared_ptr<model::Position> &pos_) {
    return std::to_string(pos_->getAccount())+":"+pos_->getSymbol();
}

std::string getOrderKey(const std::shared_ptr<model::Order> &order_) {
    return order_->getOrderID();
}
