// This file encapsulates the std::queue, providing a thread-safe way to access the queue
#ifndef _QUEUE_H
#define _QUEUE_H

#include <list>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

template <typename T, typename C = list<T> >
class safe_queue
{
private:
    queue<T, C> q;
    mutable mutex m;
    condition_variable cv;

public:
    safe_queue() = default;
    ~safe_queue() = default;

    bool empty() const
    {
        lock_guard<mutex> l(m);
        return q.empty();
    }

    size_t size() const
    {
        lock_guard<mutex> l(m);
        return q.size();
    }

    void push(const T &value)
    {
        {
            lock_guard<mutex> l(m);
            q.push(value);
        }
        cv.notify_one();
    }

    void push(T &&value)
    {
        {
            lock_guard<mutex> l(m);
            q.push(forward<T>(value));
        }
        cv.notify_one();
    }

    template <class... Args>
    void emplace(Args &&...args)
    {
        {
            lock_guard<mutex> l(m);
            q.emplace(forward<Args>(args)...);
        }
        cv.notify_one();
    }

    shared_ptr<T> pop()
    {
        unique_lock<mutex> l(m);
        cv.wait(l, [this]{ return this->q.size() > 0; });
        shared_ptr<T> ptr=make_shared<T>(q.front());
        q.pop();
        return ptr;
    }
};

#endif