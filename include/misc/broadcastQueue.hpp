// This file encapsulates the std::queue, providing a thread-safe way to access the queue
#ifndef _QUEUE_H
#define _QUEUE_H

#include <misc/util.hpp>
#include <mynet/broadcast.h>

#include <set>
#include <mutex>

using namespace std;

typedef struct limitedBroadcast
{
    uint32_t transmits; // Number of transmissions attempted.
    uint32_t msgLen;
    uint32_t id; // unique incrementing id stamped at submission time

    memberlistBroadcast b;

    limitedBroadcast(uint32_t transmits_, uint32_t msgLen_, uint32_t id_) : transmits(transmits_), msgLen(msgLen_), id(id_){};
    limitedBroadcast(uint32_t transmits_, uint32_t msgLen_, uint32_t id_, const memberlistBroadcast &b_) : transmits(transmits_), msgLen(msgLen_), id(id_), b(b_){};

    bool operator<(const limitedBroadcast &lb)
    {

        if (transmits != lb.transmits)
        {
            return transmits < lb.transmits;
        }

        if (msgLen != lb.msgLen)
        {
            return msgLen > lb.msgLen;
        }

        return id > lb.id;
    }

} limitedBroadcast;

class broadcastQueue
{

public:
    size_t size()
    {
        lock_guard<mutex> l(m);
        return rb.size();
    }

    void clear()
    {
        lock_guard<mutex> l(m);
        idGen = 0;
        for (auto i = rb.begin(); i != rb.end(); i++)
        {
            i->b.Finished();
        }
        rb.clear();
    }

    void Prune(uint32_t maxRetain);

    void QueueBroadcast(const memberlistBroadcast &b) { QueueBroadcast(b, 0); };

    string GetBroadcasts(size_t limit);

    uint8_t RetransmitMult;
    function<uint32_t()> EstNumNodes;

private:
    //smoe helper function
    void deleteItem(set<limitedBroadcast>::iterator &cur);
    void addItem(const limitedBroadcast &lb);
    void queueBroadcast(const memberlistBroadcast &b, uint32_t initialTransmits);

    //private number
    uint32_t idGen;
    set<limitedBroadcast> rb;
    mutex m;
};

// queueBroadcast is like QueueBroadcast but you can use a nonzero value for
// the initial transmit tier assigned to the message.
void broadcastQueue::queueBroadcast(const memberlistBroadcast &b, uint32_t initialTransmits)
{
    lock_guard<mutex> l(m);

    if (idGen == UINT32_MAX)
    {
        idGen = 1;
    }
    else
    {
        idGen++;
    }

    uint64_t id = idGen;

    limitedBroadcast lb(initialTransmits, b.msg.size(), id, b);

    vector<set<limitedBroadcast>::iterator> remove;
    for (auto i = rb.begin(); i != rb.end(); i++)
    {
        if (b.Invaidate(i->b))
        {
            deleteItem(i);
        }
    }

    addItem(lb);
}

// GetBroadcasts is used to get a number of broadcasts, up to a limit bytes
// and applying a per-message overhead as provided.
string broadcastQueue::GetBroadcasts(size_t overhead, size_t limit)
{
    lock_guard<mutex> l(m);

    if (rb.empty())
    {
        return NULL;
    }

    uint32_t transmitLimit = retransmitLimit(RetransmitMult, EstNumNodes());

    vector<limitedBroadcast> reinsert;
    string toSend;
    size_t bytesUsed = 0;

    uint32_t minTr = rb.begin()->transmits;
    uint32_t maxTr = rb.rbegin()->transmits;

    for (int i = minTr; i <= maxTr; i++)
    {

        while (true)
        {
            size_t free = limit - overhead - bytesUsed;

            if (free <= 0)
            {
                goto finish;
            }

            //[lower_bound,upper_bound)
            limitedBroadcast lower_bound(i, free, UINT32_MAX);
            limitedBroadcast upper_bound(i + 1, SIZE_MAX, UINT32_MAX);
            auto it_low = rb.lower_bound(lower_bound);
            auto it_up = rb.upper_bound(upper_bound);
            if (it_low == it_up)
            {
                break;
            }
            else
            {
                limitedBroadcast cur = *it_low;
                rb.erase(it_low);
                toSend += cur.b.Message();
                bytesUsed += cur.msgLen;
                cur.transmits++;
                if (cur.transmits >= transmitLimit)
                {
                    cur.b.Finished();
                }
                else
                {
                    reinsert.push_back(cur);
                }
            }
        }
    }
finish:
    for (size_t i = 0; i < reinsert.size(); i++)
    {
        rb.insert(reinsert[i]);
    }
}

// deleteItem removes the given item from the overall datastructure. You
// must already hold the mutex.
void broadcastQueue::deleteItem(set<limitedBroadcast>::iterator &cur)
{
    rb.erase(cur);

    // At idle there's no reason to let the id generator keep going
    // indefinitely.
    if (rb.empty())
    {
        idGen == 0;
    }
}

// addItem adds the given item into the overall datastructure. You must already
// hold the mutex.
void broadcastQueue::addItem(const limitedBroadcast &lb)
{
    rb.insert(lb);
}

// Prune will retain the maxRetain latest messages, and the rest
// will be discarded. This can be used to prevent unbounded queue sizes
void broadcastQueue::Prune(uint32_t maxRetain)
{
    lock_guard<mutex> l(m);

    size_t s = rb.size();

    for (auto i = rb.begin(); i != rb.end() && s > maxRetain; i++)
    {
        i->b.Finished();
        deleteItem(i);
        s--;
    }
}

#endif