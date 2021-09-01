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

    void set_interval(int interval_) { _interval = interval_; }

    void start(int interval, std::function<void(void)> func) {
        _execute = true;
        _interval = interval;
        auto thread_name = "callback_"+std::to_string(_interval);
       pthread_setname_np(pthread_self(), thread_name.c_str());

        LOGWARN("Callback timer is starting" << LOG_VAR(interval));
        std::thread([this, func, interval]() {
            while (_execute) {
                func();
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(interval));
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
