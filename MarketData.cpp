//
// Created by rory on 14/06/2021.
//


#include "MarketData.h"

std::string MarketData::getConnectionString() {
    return _connectionString + "/realtime?subscribe=trade:" + _symbol + ",instrument:"+_symbol + ",quote:"+_symbol;
}

std::shared_ptr<Event> MarketData::read() {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto event= _eventBuffer.empty() ? nullptr : _eventBuffer.top();
    if (event) {
        std::cout << "event_queue.size()=" << _eventBuffer.size() << "    ";
        _eventBuffer.pop();
    }
    return event;
}

MarketData::~MarketData() {
    _wsClient->close();
}

MarketData::MarketData(std::string connectionString_, std::string symbol_)
:   _eventBuffer()
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
                    return;
                }
                if (msgJson.has_field("success") && msgJson.at("success").as_bool()) {
                    _initialised = true;
                    return;
                }

                const std::string &table = msgJson.at("table").as_string();
                const std::string &action = msgJson.at("action").as_string();
                web::json::array &data = msgJson.at("data").as_array();
                if (table == "quote") {
                    auto qts = getData<model::Quote>(data);
                    update(qts);
                } else if (table == "trade") {
                    auto trds = getData<model::Trade>(data);
                    update(trds);
                }
            });

    _wsClient->set_close_handler(
            [](ws::client::websocket_close_status stat, const std::string& message_, std::error_code err_) { std::cout << message_ << '\n';}
    );
    std::string conString = getConnectionString();
    _wsClient->connect(conString);
    while (not _initialised)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

template<typename T>
void MarketData::update(const std::vector<std::shared_ptr<T>> &data_) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    for (const auto& row : data_)
        _eventBuffer.push(std::make_shared<Event>(row));
}

template<typename T>
void MarketData::remove(const std::vector<std::shared_ptr<T>> &data_) {
}
