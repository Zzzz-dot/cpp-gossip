#include <misc/timer.h>

using namespace std;

int64_t pushPullScale(int64_t timeinterval_, int64_t n)
{
    if (n <= pushPullScaleThreshold)
    {
        return timeinterval_;
    }
    uint32_t multiplier = ceil(log2(n) - log2(pushPullScaleThreshold)) + 1;
    return timeinterval_ * multiplier;
}

timer::timer(int64_t timeinterval_, function<void()> task_, function<uint32_t()> estNumNodes_, bool scalable_)
: running(false), timeinterval(timeinterval_), task(task_), scalable(scalable_), estNumNodes(estNumNodes_)
{};

timer::timer(const timer &t)
: running(false), timeinterval(t.timeinterval), task(t.task), scalable(t.scalable), estNumNodes(t.estNumNodes)
{};

void timer::Restart(int64_t timeinterval_){
    Stop();
    if(timeinterval_>0){
        timeinterval=timeinterval_;
    }
    Run();
}

repeatTimer::repeatTimer(int64_t timeinterval_, function<void()> task_, function<uint32_t()> estNumNodes_, bool scalable_)
:timer(timeinterval_,task_,estNumNodes_,scalable_){};

repeatTimer::repeatTimer(const timer &t) 
:timer(t){};

repeatTimer::~repeatTimer()
{
    Stop();
};

void repeatTimer::Run()
{
    // already running
    if (running.load())
    {
        return;
    }
    running.store(true);
    auto runner = [this]
    {
        while (true)
        {
            auto wait_time = this->timeinterval;
            if (this->scalable)
                wait_time = pushPullScale(wait_time, this->estNumNodes());
            this_thread::sleep_for(chrono::microseconds(wait_time));
            this->task();
        }
    };
    //If there exists a previous running thread
    if(t.joinable()){
        t.join();
    }
    t = thread(runner);
};

//return false if the timer has alreadly expired or stop
bool repeatTimer::Stop()
{
    if(running.load()==false){
        return false;
    }
    pthread_cancel(t.native_handle());
    running.store(false);
    if (t.joinable())
    {
        t.join();
    }
    return true;
};

onceTimer::onceTimer(int64_t timeinterval_, function<void()> task_, function<uint32_t()> estNumNodes_, bool scalable_)
:timer(timeinterval_,task_,estNumNodes_,scalable_){};

onceTimer::onceTimer(const timer &t)
: timer(t){};

onceTimer::~onceTimer()
{
    Stop();
}

void onceTimer::Run()
{
    // already running
    if (running.load())
    {
        return;
    }
    running.store(true);
    auto runner = [this]
    {
        auto wait_time = this->timeinterval;
        if (this->scalable)
            wait_time = pushPullScale(wait_time, this->estNumNodes());
        this_thread::sleep_for(chrono::microseconds(wait_time));
        this->task();
        this->running.store(false);
    };
    //If there exists a previous running thread
    if(t.joinable()){
        t.join();
    }
    t = thread(runner);
};

bool onceTimer::Stop()
{
    if(running.load()==false){
        return false;
    }
    pthread_cancel(t.native_handle());
    running.store(false);
    if (t.joinable())
    {
        t.join();
    }
    return true;
};
