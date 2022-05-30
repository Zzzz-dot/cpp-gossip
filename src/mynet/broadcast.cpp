#include <mynet/broadcast.h>

using namespace std;

memberlistBroadcast::memberlistBroadcast(const memberlistBroadcast &b) : node(b.node), msg(b.msg), notifyFd(b.notifyFd){};
memberlistBroadcast::memberlistBroadcast(const string &node_, const Broadcast &msg_, int notifyFd_) : node(node_), msg(msg_), notifyFd(notifyFd_){};

bool memberlistBroadcast::Invaidate(const memberlistBroadcast &m) const
{
    return node == m.node;
};

void memberlistBroadcast::Finished() const
{
    if (notifyFd != -1)
    {
        Close(notifyFd);
    }
};

Broadcast memberlistBroadcast::Message() const
{
    return msg;
};

string memberlistBroadcast::Name() const{
    return node;
}
