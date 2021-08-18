//
// Created by Rory McStay on 18/08/2021.
//

#ifndef MY_PROJECT_CALLBACKTIMER_H
#define MY_PROJECT_CALLBACKTIMER_H

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

class CallbackTimer
{
public:
    CallbackTimer()
    :   _execute(false)
    {}

    void start(int interval, std::function<void(void)> func) {
        _execute = true;
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

private:
    bool            _execute;
};

#endif //MY_PROJECT_CALLBACKTIMER_H
