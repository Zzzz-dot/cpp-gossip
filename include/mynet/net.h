#ifndef _NET_H
#define _NET_H

#include "wrapped.h"

#include <type/msgtype.pb.h>
#include <memberlist/memberlist.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

MessageData onReceive(const string &s);

MessageData onReceiveUDP(int fd,sockaddr_in &remote_addr,socklen_t &sin_size);

void beforeSend(const MessageData &md, string *s);

string beforeSend(const MessageData &md);

void sendTCP(const struct sockaddr_in *remote_addr, const void *msg, size_t n);

void sendUDP(int fd, const struct sockaddr_in *remote_addr, const void *msg, size_t n);

void encodeSendTCP(const struct sockaddr_in *remote_addr, const MessageData &md);

void encodeSendUDP(int fd, const struct sockaddr_in *remote_addr, const MessageData &md);

#endif