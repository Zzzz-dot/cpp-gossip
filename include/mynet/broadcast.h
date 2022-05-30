#ifndef _BROADCAST_H
#define _BROADCAST_H


#include "wrapped.h"

#include <type/msgtype.pb.h>

#include <iostream>

struct memberlistBroadcast
{
    memberlistBroadcast()=default;
    memberlistBroadcast(const memberlistBroadcast &b);
    memberlistBroadcast(const std::string &node_, const Broadcast &msg_, int notifyFd_);

    bool Invaidate(const memberlistBroadcast &m) const;
    void Finished() const;
    Broadcast Message() const;
    std::string Name() const;

private:
    std::string node;
    Broadcast msg;

    int notifyFd;
};

#endif