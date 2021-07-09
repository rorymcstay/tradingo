//
// Created by rory on 08/07/2021.
//

#include "HeartBeat.h"

#include <utility>
#include "Utils.h"

void HeartBeat::startTimer(long cycle_) {
    _cycle = cycle_;
}

HeartBeat::HeartBeat(std::shared_ptr<web::web_sockets::client::websocket_callback_client> client_)
:   _wsClient(std::move(client_))
,   _cycle(0)
,   _timerThread() {


}

void HeartBeat::sendPing() {
    web::web_sockets::client::websocket_outgoing_message message;
    message.set_ping_message("ping");
    _wsClient->send(message);
}

void HeartBeat::init() {

    auto timedPong = [this]() {
        long cycle;
        while (_wsClient) {
            cycle = _cycle;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            // if cycle has not changes after 5 seconds
            if (_cycle == cycle) {
                LOGWARN("Sending ping message after 5 seconds.");
                sendPing();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (_cycle == cycle) {
                    LOGWARN("Reconnecting to " << LOG_NVP("webSocketUrl", _wsClient->uri().to_string()));
                    // cycle not changed after ping
                    _wsClient->connect(_wsClient->uri().to_string());
                }
                //throw HeartBeatTimeOut("Timed out after sending heartbeat after 5 seconds.");
                LOGINFO("Pong received after 5 seconds");
            }
        }
    };
    _timerThread = std::thread(timedPong);
    LOGINFO("Initiated HeartBeatThread");
}
