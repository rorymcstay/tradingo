//
// Created by rory on 14/06/2021.
//

#include "MarketData.h"
#include "Utils.h"
#include "Config.h"
#include <model/Position.h>
#include <model/Order.h>
#include <model/OrderBookL2.h>

#include <auth_helpers.h>

#include <utility>
#include "Functional.h"
#include "InstrumentService.h"

#include "OrderBookUtils.h"


std::string MarketData::getConnectionUri() {
    // TODO handle auth
    return "/realtime/websocket";
}

std::string MarketData::getBaseUrl() {
    return _connectionString;
}

std::shared_ptr<Event> MarketDataInterface::read() {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto event= _eventBuffer.empty() ? nullptr : _eventBuffer.top();
    if (event) {
        //INFO(LOG_NVP("Queue Size", _eventBuffer.size()));
        _eventBuffer.pop();
    }
    return event;
}

MarketData::~MarketData() {
    _wsClient->close();
}

MarketData::MarketData(const std::shared_ptr<Config>& config_, std::shared_ptr<InstrumentService> instrumentSvc_)
:   MarketDataInterface(config_, instrumentSvc_)
,   _connectionString(config_->get<std::string>("connectionString"))
,   _apiKey(config_->get<std::string>("apiKey", "NO_AUTH"))
,   _apiSecret(config_->get<std::string>("apiSecret", "NO_AUTH"))
,   _wsClient(std::make_shared<ws::client::websocket_callback_client>())
,   _initialised(false)
,   _shouldAuth(config_->get<bool>("shouldAuth", true))
,   _cancelAllAfter(config_->get<bool>("cancelAllAfter", true))
,   _cancelAllTimeout(config_->get<int>("cancelAllTimeout", 15000))
,   _heartBeat(nullptr)
,   _cycle(0)
{
    ;
}

/// initialise heartbeat, callback method
void MarketData::init() {
    _heartBeat = std::make_shared<HeartBeat>(_wsClient);

    // initialise callback
    _wsClient->set_message_handler(
            [&](const ws::client::websocket_incoming_message& in_msg) {
                // TODO Move this logic into handler method.
                _heartBeat->startTimer(_cycle++);
                auto msg = in_msg.extract_string();
                auto stringVal = msg.get();
                if (stringVal.find("ping") != std::string::npos
                         || stringVal.find("pong") != std::string::npos) {
                    return;
                }
                web::json::value msgJson = web::json::value::parse(stringVal);
                if (msgJson.has_field("table")) {

                    const std::string &table = msgJson.at("table").as_string();
                    const std::string &action = msgJson.at("action").as_string();
                    web::json::array &data = msgJson.at("data").as_array();

                    if (table == "quote") {
                        auto quotes = getData<model::Quote>(data);
                        handleQuotes(quotes, action);
                    } else if (table == "trade") {
                        auto trades = getData<model::Trade>(data);
                        handleTrades(trades, action);
                    } else if (table == "execution") {
                        auto exec = getData<model::Execution>(data);
                        handleExecutions(exec, action);
                    } else if (table == "position") {
                        auto positions = getData<model::Position>(data);
                        handlePositions(positions, action);
                    } else if (table == "order") {
                        auto orders = getData<model::Order>(data);
                        handleOrders(orders, action);
                    } else if (table == "margin") {
                        auto margins = getData<model::Margin>(data);
                        handleMargin(margins, action);
                    } else if (table == "instrument") {
                        auto instruments = getData<model::Instrument>(data);
                        handleInstruments(instruments, action);
                    } else if (
                            table == "orderBookL2_25"
                            || table == "orderBookL2"
                            || table == "orderBookL10") {
                        auto updates = getData<model::OrderBookL2>(data);
                        handleOrderBookL2(updates, action);
                    }
                } else if (msgJson.has_field("info")) {
                    LOGINFO("Connection response: " << msgJson.serialize());
                    _initialised = true;
                    return;
                } else if (msgJson.has_field("success") && msgJson.at("success").as_bool()) {
                    LOGINFO("Operation success: " << msgJson.serialize());
                    return;
                } else if (msgJson.has_field("error")) {
                    LOGWARN("Websocket error :" << stringVal);
                    return;
                } else if (msgJson.has_field("cancelTime")) {
                    LOGWARN("CancelAfter success: " << stringVal);
                }
                callback();
            });

    _wsClient->set_close_handler(
            [](ws::client::websocket_close_status stat, const std::string& message_, std::error_code err_) {
                LOGINFO("Websocket closed!");
            });

    std::string conString = getBaseUrl() + getConnectionUri();
    LOGINFO("Connecting to " << LOG_VAR(conString));
    _wsClient->connect(conString);

    // sleep until connected.
    int count = 0;
    while (not _initialised) {
        count++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (count > 30) {
            LOGWARN("Timed out after 30 seconds waiting for log on message.");
            throw std::runtime_error("timed out after 30 seconds connecting to "+ conString);
        }
    }

    // Authorise
    if (_shouldAuth) {
        std::string expires = std::to_string(getExpires());
        std::string signature = hex_hmac_sha256(_apiSecret, "GET/realtime" + expires);
        std::string payload =
                R"({"op": "authKeyExpires", "args": [")" + _apiKey + R"(", )" + expires + R"(, ")" + signature + R"("]})";
        LOGINFO("Authenticating on websocket " << LOG_VAR(payload));
        web::websockets::client::websocket_outgoing_message message;
        message.set_utf8_message(payload);
        _wsClient->send(message);
    }

    auto timeoutCallback = [this] () {
        reconnect();
    };
    _heartBeat->init(timeoutCallback);

}

/// subscribe to topics based on 'shouldAuth' config value.
/// if 'cancelAllAfter' is true, register it with server.
void MarketData::subscribe() {

    std::vector<std::string> default_topics = {
            "position",
            "order",
            "margin",
            "execution:" + _symbol,
            "quote:"+_symbol,
            "trade:"+_symbol,
            "instrument:"+_symbol,

    };

    std::vector<std::string> topics = _config->get("marketdata-topics", default_topics);

    auto payload = web::json::value::parse(R"({"op": "subscribe", "args": []})");
    int num = 0;
    for (auto& topic : topics) {
        payload.at("args").as_array()[num++] = web::json::value(topic);
    }
    web::websockets::client::websocket_outgoing_message subMessage;
    subMessage.set_utf8_message(payload.serialize());
    LOGINFO("Doing subscribe " << payload.serialize());
    _wsClient->send(subMessage);
    if (_cancelAllAfter && _shouldAuth) {
        web::websockets::client::websocket_outgoing_message killSwitchMessage;
        std::string data = R"({"op": "cancelAllAfter", "args": [)" + std::to_string(_cancelAllTimeout) +"]}";
        LOGINFO("Registering kill switch " << LOG_VAR(data));
        killSwitchMessage.set_utf8_message(data);
        _wsClient->send(killSwitchMessage);
    }
}

void MarketData::reconnect() {
    LOGINFO("Reinitialising websocket client " << _wsClient);
    _heartBeat->stop();
    _wsClient->close();
    _wsClient = std::shared_ptr<ws::client::websocket_callback_client>();
    init();
    subscribe();
}

template<typename T>
void MarketDataInterface::update(const std::vector<std::shared_ptr<T>> &data_) {
    for (const auto& row : data_) {
        _eventBuffer.push(std::make_shared<Event>(row));
    }
}

template<>
void MarketDataInterface::update(const std::vector<std::shared_ptr<model::Instrument>> &data_) {
    for (const auto& row : data_) {
        auto instrument = _instruments[row->getSymbol()];
        _eventBuffer.push(std::make_shared<Event>(instrument, row));
    }
}

void MarketDataInterface::updatePositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {
    for (auto& pos : positions_) {
        auto update_json = pos->toJson();
        _positions[getPositionKey(pos)]->fromJson(update_json);
    }
}

void MarketDataInterface::insertPositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {

    for (auto& pos : positions_) {
        _positions[getPositionKey(pos)] = pos;
    }
}

void MarketDataInterface::removePositions(const std::vector<std::shared_ptr<model::Position>>& positions_) {

    for (auto& pos : positions_)
        _positions.erase(getPositionKey(pos));
}

void MarketDataInterface::handleQuotes(const std::vector<std::shared_ptr<model::Quote>>& quotes_, const std::string &action_) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    _quote = quotes_[0];
    update(quotes_);
}

void MarketDataInterface::handleTrades(std::vector<std::shared_ptr<model::Trade>>& trades_, const std::string &action_) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
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

void
MarketDataInterface::handleMargin(std::vector<std::shared_ptr<model::Margin>> margin_, const std::string &action_) {
    if (action_ == "update") {
        auto update_json = margin_[0]->toJson();
        _margin->fromJson(update_json);
    } else if (action_ == "partial") {
        // update margin
        _margin = margin_[0];
    } else if (action_ == "insert") {
        _margin = margin_[0];
    }

}

void MarketDataInterface::handleExecutions(std::vector<std::shared_ptr<model::Execution>> &execs_,
                                           const std::string &action_) {
    if (action_ == "update" or action_ == "insert" or action_ == "partial") {
        for (auto& ex : execs_)
            _executions.push(ex);
    }
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    update(execs_);
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


void MarketDataInterface::handleInstruments(
    const std::vector<std::shared_ptr<model::Instrument>>& instruments_,
    const std::string& action_) {
    for (auto& instrument : instruments_) {
        auto symbol = instrument->getSymbol();
        if (action_ == "update" and _instruments.find(symbol) != _instruments.end()) {
            auto update_json = instrument->toJson();
            _instruments[symbol]->fromJson(update_json);
        } else if (action_ == "insert" or action_ == "update" or action_=="partial") {
            _instruments[symbol] = instrument;
        } else if (action_ == "delete") {
            _instruments.erase(symbol);
        }
        update(instruments_);
    }
}

void MarketDataInterface::handleOrderBookL2(
        const std::vector<std::shared_ptr<model::OrderBookL2>>& updates_,
        const std::string& action_) {

    if (action_ == "insert" || action_ == "partial") {
        for (auto& update : updates_) {
            _symbolIdx = tradingo_utils::orderbook_l2::symbolIdx(
                    update->getPrice(),
                    update->getId(),
                    _tickSize);
            break;
        }
    }
    if (action_ == "delete") {
        for (auto& update : updates_) {
            auto price = tradingo_utils::orderbook_l2::price(
                        _symbolIdx, update->getId(), _tickSize);
            _orderBook.removeLevel(price);
            update->setSize(0.0);
        }
        return;
    }
    for (auto& update : updates_) {
        auto price = tradingo_utils::orderbook_l2::price(
                    _symbolIdx, update->getId(), _tickSize);
        _orderBook.updateLevel(price, update);
    }

}

void
MarketDataInterface::removeOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_)
        _orders.erase(getOrderKey(ord));
}

void
MarketDataInterface::insertOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_) {
        _orders.insert(std::pair(getOrderKey(ord), ord));
    }
}

void
MarketDataInterface::updateOrders(const std::vector<std::shared_ptr<model::Order>> &orders_) {
    for (auto& ord : orders_) {
        auto update_json = ord->toJson();
        _orders.at(getOrderKey(ord))->fromJson(update_json);
    }
}

const std::unordered_map<std::string, MarketDataInterface::OrderPtr>&
MarketDataInterface::getOrders() const {
    return _orders;
}

const std::queue<MarketDataInterface::ExecPtr>&
MarketDataInterface::getExecutions() const {
    return _executions;
}

const std::unordered_map<std::string, MarketDataInterface::PositionPtr>&
MarketDataInterface::getPositions() const {
    return _positions;
}
const MarketDataInterface::MarginPtr
&MarketDataInterface::getMargin() const {
    return _margin;
}

void
MarketDataInterface::updateSignals(const std::shared_ptr<Event>& event_) {
    /*
    for (auto& signal : _timed_signals) {
        signal.second->update(event_);
    }*/
}

void MarketDataInterface::initSignals(const std::string& config) {
    auto conf = std::make_shared<Config>(config);
}

const std::shared_ptr<model::Quote>
MarketDataInterface::quote() const {
    return _quote;
}

const std::shared_ptr<model::Instrument>&
MarketDataInterface::instrument() const {
    return _instruments.at(_symbol);
}

const std::unordered_map<std::string, std::shared_ptr<model::Instrument>>&
MarketDataInterface::getInstruments() const {
    return _instruments;
}


MarketDataInterface::MarketDataInterface(const std::shared_ptr<Config>& config_,
                                         std::shared_ptr<InstrumentService>  instSvc_)
:   _instSvc(std::move(instSvc_))
,   _config(config_)
,   _symbol(config_->get<std::string>("symbol"))
,   _tickSize(0.01)
,   _callback([]() {})
,   _orderBook(_symbol, 30000.0, _tickSize, 100.0)
{

}

void MarketDataInterface::setCallback(const std::function<void()> &callback) {
    _callback = callback;
}

MarketDataInterface::MarketDataInterface()
:   _instSvc(nullptr)
,   _symbol("XBTUSD")
,   _tickSize(0.01)
,   _orderBook(_symbol, 30000.0, _tickSize, 100.0)
,   _callback([]() {}) {

}


std::string getPositionKey(const std::shared_ptr<model::Position> &pos_) {
    return pos_->getSymbol();
}

std::string getOrderKey(const std::shared_ptr<model::Order> &order_) {
    return order_->getOrderID();
}
