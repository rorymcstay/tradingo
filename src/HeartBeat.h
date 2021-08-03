//
// Created by rory on 08/07/2021.
//

#ifndef MY_PROJECT_HEARTBEAT_H
#define MY_PROJECT_HEARTBEAT_H
#include "cpprest/ws_client.h"
#include "cpprest/ws_msg.h"
#include <thread>


/*
    After receiving each message, set a timer a duration of 5 seconds.
    If any message is received before that timer fires, restart the timer.
    When the timer fires (no messages received in 5 seconds), send a raw ping frame (if supported) or the literal string 'ping'.
    Expect a raw pong frame or the literal string 'pong' in response. If this is not received within 5 seconds, throw an error or reconnect.
 */

class HeartBeatTimeOut : std::runtime_error {

public:
    HeartBeatTimeOut(std::string message_) : std::runtime_error(message_) {}

};

class HeartBeat {

    std::shared_ptr<web::web_sockets::client::websocket_callback_client> _wsClient;
    std::thread _timerThread;
    long _cycle;
    bool _stop;

public:
    HeartBeat(std::shared_ptr<web::web_sockets::client::websocket_callback_client>);
    void startTimer(long cycle_);
    void sendPing();
    void init(const std::function<void()>& timeoutCallback_);
    void stop();

};


#endif //MY_PROJECT_HEARTBEAT_H
