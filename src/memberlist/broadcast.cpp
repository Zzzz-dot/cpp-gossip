#include <memberlist/memberlist.h>

using namespace std;

// encodeAndBroadcast encodes a message and enqueues it for broadcast. Fails
// silently if there is an encoding error.
void memberlist::onlyBroadcast(string node, const Broadcast &bc)
{
    BroadcastNotify(node, bc, -1);
}

// encodeBroadcastNotify encodes a message and enqueues it for broadcast
// and notifies the given channel when transmission is finished. Fails
// silently if there is an encoding error.
void memberlist::BroadcastNotify(string node, const Broadcast &bc, int notifyFd)
{

    memberlistBroadcast b(node, bc, notifyFd);

    TransmitLimitedQueue->QueueBroadcast(b);
}

// getBroadcasts is used to return a slice of broadcasts to send up to
// a maximum byte size, while imposing a per-broadcast overhead. This is used
// to fill a UDP packet with piggybacked data
bool memberlist::getBroadcasts(size_t overhead, size_t limit, ComBroadcast &cbc)
{
    bool haveMsg = TransmitLimitedQueue->GetBroadcasts(overhead, limit, cbc);
    if (!haveMsg)
    {
        return false;
    }
    return true;
}