// This file encapsulates the std::queue, providing a thread-safe way to access the queue
#ifndef _QUEUE_H
#define _QUEUE_H

#include "util.h"

#include <cmakeConfig.h>
#include <mynet/broadcast.h>
#include <type/genmsg.h>
#include <glog/logging.h>

#include <set>
#include <map>
#include <mutex>

struct limitedBroadcast
{
    uint32_t transmits; // Number of transmissions attempted.
    size_t msgLen;
    uint32_t id; // unique incrementing id stamped at submission time

    memberlistBroadcast b;

    limitedBroadcast()=default;
    limitedBroadcast(uint32_t transmits_, size_t msgLen_, uint32_t id_);
    limitedBroadcast(uint32_t transmits_, size_t msgLen_, uint32_t id_, const memberlistBroadcast &b_);
};

bool operator<(const limitedBroadcast &lb1,const limitedBroadcast &lb2);

class broadcastQueue
{

public:
    size_t size();

    void clear();

    void Prune(uint32_t maxRetain);

    void QueueBroadcast(const memberlistBroadcast &b);

    bool GetBroadcasts(size_t overhead, size_t limit, ComBroadcast &cbc);

    uint8_t RetransmitMult;
    std::function<uint32_t()> EstNumNodes;

    broadcastQueue(uint8_t RetransmitMult_, std::function<uint32_t()> EstNumNodes_);

private:
    // smoe helper function
    void deleteItem(const limitedBroadcast &lb);
    void addItem(const limitedBroadcast &lb);
    void queueBroadcast(const memberlistBroadcast &b, uint32_t initialTransmits);

    // private number
    uint32_t idGen;
    std::set<limitedBroadcast> tq;
    std::map<std::string,limitedBroadcast> tm;
    std::mutex m;

#ifdef DEBUG
    void orderedView();
#endif
};

#endif