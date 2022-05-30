// Create a repeatTimer with the given function f(), and f() will be excuted periodically until exit
#ifndef _TIMER_H
#define _TIMER_H

#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <cmath>
#include <atomic>

#define pushPullScaleThreshold 32

class timer
{
public:
    virtual void Run() = 0;
    virtual bool Stop() = 0;
    void Restart(int64_t timeinterval_ = 0);

    //timer()=default;
    timer(int64_t timeinterval_, std::function<void()> task_, std::function<uint32_t()> estNumNodes_, bool scalable_ = false);
    timer(const timer &t);

protected:
    std::atomic<bool> running;
    bool scalable;
    int64_t timeinterval;
    std::function<void()> task;
    std::thread t;
    std::function<uint32_t()> estNumNodes;
};

// This timer will run task only once and then stop, or it can be stopped prematurely
class onceTimer:public timer
{
public:
    //onceTimer() = default;
    onceTimer(int64_t timeinterval_, std::function<void()> task_, std::function<uint32_t()> estNumNodes_, bool scalable_=false);
    onceTimer(const timer &t);

    ~onceTimer();

    void Run();

    bool Stop();
};

// This timer will repeatedly run task until being stopped
class repeatTimer : public timer
{
public:
    //repeatTimer()=default;
    repeatTimer(int64_t timeinterval_, std::function<void()> task_, std::function<uint32_t()> estNumNodes_, bool scalable_ = false);
    repeatTimer(const timer &t);

    ~repeatTimer();

    void Run();

    bool Stop();
};

#endif