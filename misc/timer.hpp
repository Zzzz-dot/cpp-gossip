// Create a timer with the given function f(), and f() will be excuted periodically until exit
#ifndef _TIMER_H
#define _TIMER_H
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <cmath>

#include "../memberlist/memberlist.h"

using namespace std;

#define pushPullScaleThreshold 32
uint32_t pushPullScale(uint32_t timeinterval_,uint32_t n){
    if (n<=pushPullScaleThreshold){
        return timeinterval_;
    }
    uint32_t multiplier=ceil(log2(n)-log2(pushPullScaleThreshold))+1;
    return timeinterval_*multiplier;
}

class timer
{
public:
    timer(uint32_t timeinterval_, function<void()> task_, memberlist *memb_, bool scalable_ = false) : running(false), timeinterval(timeinterval_), task(task_), scalable(scalable_), memb(memb_){};
    ~timer()
    {
        Stop();
    };

    uint32_t estNumNodes()
    {
        return memb->numNodes.load();
    };

    void Run()
    {
        running = true;
        auto runner = [this]
        {
            while (this->running)
            {
                {
                    unique_lock<mutex> l(this->m);
                    
                    auto wait_time = this->timeinterval;
                    if(this->scalable)
                        wait_time=pushPullScale(wait_time,this->estNumNodes());
                    this->cv.wait_for(l, chrono::milliseconds(wait_time), [this]
                                      { return !this->running; });
                }
                if (!this->running)
                {
                    return;
                }
                this->task();
            }
        };
        t = thread(runner);
    };

    void Stop()
    {
        {
            lock_guard<mutex> l(m);
            running = false;
        }
        cv.notify_one();
        t.join();
    };

private:
    bool running;
    bool scalable;
    uint32_t timeinterval; // milliseconds
    function<void()> task;
    mutex m;
    condition_variable cv;
    thread t;

    memberlist *memb;
};

#endif