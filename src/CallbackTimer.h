//
// Created by Rory McStay on 18/08/2021.
//

#ifndef MY_PROJECT_CALLBACKTIMER_H
#define MY_PROJECT_CALLBACKTIMER_H

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>
#include <thread>
#include <pthread.h>

#include "Utils.h"

class CallbackTimer
{
public:
    CallbackTimer()
    :   _execute(false)
    ,   _interval(0)
    {}

    void start(int interval, std::function<void(void)> func) {
        _execute = true;
        _interval = interval;
        auto thread_name = "callback_"+std::to_string(_interval);
       pthread_setname_np(pthread_self(), thread_name.c_str());

#ifdef REPLAY_MODE
        auto to_sleep = interval/10;
        LOGWARN("Replay Mode is activated, " << LOG_VAR(to_sleep));
#else
        auto to_sleep = interval;
        LOGWARN("Replay Mode is deactivated, " << LOG_VAR(to_sleep));

#endif
        std::thread([this, func, to_sleep]() {
            while (_execute) {
                func();
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(to_sleep));
            }
        }).detach();
    }

    void stop() {
        _execute = false;
    }
    int interval() { return _interval; }

private:
    bool            _execute;
    int _interval;
};

#endif //MY_PROJECT_CALLBACKTIMER_H
