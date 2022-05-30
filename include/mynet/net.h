#ifndef _NET_H
#define _NET_H

#include "wrapped.h"

#include <type/msgtype.pb.h>

#include <iostream>
#include <arpa/inet.h>


#define MessageDataOverhead 4

MessageData decodeReceiveUDP(int fd, sockaddr_in &remote_addr, socklen_t &sin_size);

MessageData decodeReceiveTCP(int fd);

void beforeSend(const MessageData &md, std::string *s);

std::string beforeSend(const MessageData &md);

void encodeSendTCP(int fd, const MessageData &md);

void encodeSendUDP(int fd, const sockaddr_in *remote_addr, const MessageData &md);
#endif