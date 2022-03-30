//This file encapsulates the std::queue, providing a thread-safe way to access the queue
#ifndef _QUEUE_H
#define _QUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>
using namespace std;
template <typename T>
class safe_queue:private queue<T> {
    private:
        mutex m;
        condition_variable cv;
    public:
        safe_queue()=default;
        ~safe_queue()=default;

        reference front(){
            lock_guard(m);
            return 
        }

}

#endif