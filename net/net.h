#ifndef _NET_H
#define _NET_H
#include "wrapped.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <chrono>
#include <stdlib.h>

#include "memberlist/type/msgtype.pb.h"
#include <iostream>
using namespace std;

#define SWITCH(cond)\
    switch (md.head()){\
        case MessageData_MessageType::MessageData_MessageType_pingMsg:\
            \
            break;\
        case MessageData_MessageType::MessageData_MessageType_indirectPingMsg:\
                \
            break;\
        case MessageData_MessageType::MessageData_MessageType_ackRespMsg:\
                \
            break;\
        case MessageData_MessageType::MessageData_MessageType_suspectMsg:\
            \
        case MessageData_MessageType::MessageData_MessageType_aliveMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_deadMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_pushPullMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_userMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_nackRespMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_errMsg:\
\
        default:\
    }\

void onReceive(const string &s){
    MessageData md;
    if(md.ParseFromString(s)==false){
        cout<<"ParseFromString Error!"<<endl;
        return;
    }
    SWITCH(md.head());
}

void onReceive(int fd){
    MessageData md;
    if(md.ParseFromFileDescriptor(fd)==false){
        cout<<"ParseFromFileDescriptor Error!"<<endl;
        return;
    }
    SWITCH(md.head());
}

void beforeSend(const MessageData &md,string *s){
    if(md.SerializeToString(s)==false){
        cout<<"SerializeToString Error!"<<endl;
        return;
    }
}

string beforeSend(const MessageData &md){
    string s=md.SerializeAsString();
    if (s.empty()){
        cout<<"SerializeAsString Error!"<<endl;
        return;
    }

}

void sendTCP(const struct sockaddr_in *server_addr, const void *msg, size_t n)
{
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(fd, (struct sockaddr *)server_addr, sizeof(sockaddr));
    struct sockaddr_in local_addr;
    socklen_t len;
    getsockname(fd, (struct sockaddr *)&local_addr, &len);
    char addr[INET6_ADDRSTRLEN];
    in_port_t port = ntohs(local_addr.sin_port);
    inet_ntop(AF_INET, &local_addr.sin_addr, addr, sizeof(addr));
    printf("Local Address: %s\n", addr);
    printf("Local Port: %hhu\n", port);
    sleep(10);
    Write(fd, msg, n);
}

void sendUDP(int fd,const struct sockaddr_in *server_addr, const void *msg, size_t n)
{
    Sendto(fd, msg, n, 0, (struct sockaddr *)server_addr, sizeof(sockaddr));
}

void encodeSendTCP(const struct sockaddr_in *server_addr,const MessageData &md){
    string encodeMsg=beforeSend(md);
    sendTCP(server_addr,encodeMsg.c_str(),encodeMsg.size());
};

void encodeSendUDP(int fd,const struct sockaddr_in *server_addr,const MessageData &md){
    string encodeMsg=beforeSend(md);
    sendUDP(fd,server_addr,encodeMsg.c_str(),encodeMsg.size());
};

#endif