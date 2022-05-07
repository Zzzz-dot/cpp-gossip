#ifndef _BROADCAST_H
#define _BRAADCAST_H

#include <type/msgtype.pb.h>

#include <iostream>
using namespace std;

typedef struct memberlistBroadcast
{

    memberlistBroadcast(const memberlistBroadcast &b) : node(b.node), msg(b.msg),notifyfd(b.notifyfd){};
    memberlistBroadcast(const string &node_, const string &msg_, int notifyfd_) : node(node_), msg(msg_),notifyfd(notifyfd_){};

    bool Invaidate(const memberlistBroadcast &m) const { return node == m.node; };
    bool Finished() const {};
    string Message() const { return msg; };

private:
    string node;
    string msg;

    int notifyfd;

} memberlistBroadcast;

#endif