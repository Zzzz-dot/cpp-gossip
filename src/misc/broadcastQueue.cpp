#include <misc/broadcastQueue.h>

using namespace std;

limitedBroadcast::limitedBroadcast(uint32_t transmits_, size_t msgLen_, uint32_t id_) : transmits(transmits_), msgLen(msgLen_), id(id_){};

limitedBroadcast::limitedBroadcast(uint32_t transmits_, size_t msgLen_, uint32_t id_, const memberlistBroadcast &b_) : transmits(transmits_), msgLen(msgLen_), id(id_), b(b_){};

bool operator<(const limitedBroadcast &lb1, const limitedBroadcast &lb2)
{
    if (lb1.transmits != lb2.transmits)
    {
        return lb1.transmits < lb2.transmits;
    }

    if (lb1.msgLen != lb2.msgLen)
    {
        return lb1.msgLen > lb2.msgLen;
    }

    return lb1.id > lb2.id;
}

broadcastQueue::broadcastQueue(uint8_t RetransmitMult_, function<uint32_t()> EstNumNodes_) : RetransmitMult(RetransmitMult_), EstNumNodes(EstNumNodes_), idGen(0){};

size_t broadcastQueue::size()
{
    lock_guard<mutex> l(m);
    return tq.size();
}

void broadcastQueue::clear()
{
    lock_guard<mutex> l(m);
    idGen = 0;
    for (auto i = tq.begin(); i != tq.end(); i++)
    {
        i->b.Finished();
    }
    tq.clear();
    tm.clear();
}

void broadcastQueue::QueueBroadcast(const memberlistBroadcast &b)
{
    queueBroadcast(b, 0);
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

    limitedBroadcast lb(initialTransmits, b.Message().ByteSizeLong(), id, b);

    deleteItem(lb);

    addItem(lb);

#ifdef DEBUG
    orderedView();
#endif
}

// GetBroadcasts is used to get a number of broadcasts, up to a limit bytes
// and applying a per-message overhead as provided.
bool broadcastQueue::GetBroadcasts(size_t overhead, size_t limit, ComBroadcast &cbc)
{
    lock_guard<mutex> l(m);

    if (tq.empty())
    {
        return false;
    }

    uint32_t transmitLimit = retransmitLimit(RetransmitMult, EstNumNodes());

    vector<limitedBroadcast> reinsert;
    size_t bytesUsed = 0;
    bool haveMsg;

    uint32_t minTr = tq.begin()->transmits;
    uint32_t maxTr = tq.rbegin()->transmits;

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
            auto it_low = tq.lower_bound(lower_bound);
            auto it_up = tq.upper_bound(upper_bound);
            if (it_low == it_up)
            {
                break;
            }
            else
            {
                haveMsg = true;
                limitedBroadcast cur = *it_low;
                deleteItem(cur);
                addBroadCast(cbc, cur.b.Message());
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
        addItem(reinsert[i]);
    }
#ifdef DEBUG
    orderedView();
#endif
    return haveMsg;
}

// deleteItem removes the given item from the overall datastructure. You
// must already hold the mutex.
void broadcastQueue::deleteItem(const limitedBroadcast &lb)
{
    if (tm.find(lb.b.Name()) != tm.end())
    {
        tm.erase(lb.b.Name());
        for (auto i = tq.begin(); i != tq.end();)
        {
            if (i->b.Invaidate(lb.b))
            {
                i->b.Finished();
                tq.erase(i++);
            }
            else
            {
                i++;
            }
        }
    }
    // At idle there's no reason to let the id generator keep going
    // indefinitely.
    if (tq.empty())
    {
        idGen == 0;
    }
}

// addItem adds the given item into the overall datastructure. You must already
// hold the mutex.
void broadcastQueue::addItem(const limitedBroadcast &lb)
{
    tm[lb.b.Name()] = lb;
    tq.insert(lb);
}

// Prune will retain the maxRetain latest messages, and the rest
// will be discarded. This can be used to prevent unbounded queue sizes
void broadcastQueue::Prune(uint32_t maxRetain)
{
    lock_guard<mutex> l(m);

    size_t s = tq.size();

    auto i = tq.rbegin();
    while (s > maxRetain && i != tq.rend())
    {
        tm.erase(i->b.Name());
        i->b.Finished();
        i = decltype(i)(tq.erase(next(i).base()));
        s--;
    }
}

#ifdef DEBUG
void broadcastQueue::orderedView()
{
    LOG(INFO) << "broadcastQueue orderedView: " << endl;
    size_t index = 0;
    for (auto i = tq.begin(); i != tq.end(); i++)
    {
        index++;
        LOG(INFO) << "broadcastQueue Message [" << index << "]: " << endl;
        LOG(INFO) << i->b.Message().DebugString()<< "Transmits: " << i->transmits << ", ID: " << i->id << ", Len: " << i->msgLen << endl;
    }
}
#endif