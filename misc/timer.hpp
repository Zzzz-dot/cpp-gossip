// Create a timer with the given function f(), and f() will be excuted periodically until exit
#ifndef _TIMER_H
#define _TIMER_H
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>

using namespace std;

class timer{
    public:
        timer(uint32_t timeinterval_,function<void()> task_):running(false),timeinterval(timeinterval_),task(task_){};
        ~timer(){
            Stop();
        };

        void Run(){

            running=true;
            auto runner=[this]{
                while(this->running){
                    {
                        unique_lock<mutex> l(this->m);
                        this->cv.wait_for(l,chrono::milliseconds(this->timeinterval),[this]{return !this->running;});
                    }
                    if(!this->running){
                        return;
                    }
                    this->task();
                }   
            };
            t=thread(runner);
        };

        void Stop(){
            {
                lock_guard<mutex> l(m);
                running=false;
            }
            cv.notify_one();
            t.join();
        };
    private:
        bool running;
        uint32_t timeinterval;  //milliseconds
        function<void()> task;
        mutex m;
        condition_variable cv;
        thread t;
};

#endif