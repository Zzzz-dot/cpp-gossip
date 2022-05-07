#include <mynet/broadcast.h>
#include <memberlist/memberlist.h>

// encodeAndBroadcast encodes a message and enqueues it for broadcast. Fails
// silently if there is an encoding error.
void memberlist::encodeAndBroadcast(string node, const Compound &cd) {
    encodeBroadcastNotify(node,cd,NULL);
}

// encodeBroadcastNotify encodes a message and enqueues it for broadcast
// and notifies the given channel when transmission is finished. Fails
// silently if there is an encoding error.
void memberlist::encodeBroadcastNotify(string node, const Compound &cd,int notifyfd){
    string msg=cd.SerializeAsString();
    if (msg.empty())
    {
        cout << "SerializeAsString Error!" << endl;
    }

    memberlistBroadcast b(node,msg,notifyfd);

    TransmitLimitedQueue.QueueBroadcast(b);
}

// getBroadcasts is used to return a slice of broadcasts to send up to
// a maximum byte size, while imposing a per-broadcast overhead. This is used
// to fill a UDP packet with piggybacked data
string memberlist::getBroadcasts(size_t overhead, size_t limit){
    return TransmitLimitedQueue.GetBroadcasts(overhead,limit);
}