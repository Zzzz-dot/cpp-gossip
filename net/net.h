#ifndef _NET_H
#define _NET_H
#include "wrapped.h"


void SendTcpMsg(const struct sockaddr_in& server_addr,const void *msg,size_t n){
    int fd=Socket(AF_INET,SOCK_STREAM,0);
    Connect(fd,(struct sockaddr*)&server_addr,sizeof(sockaddr));
    Write(fd,msg,n);
}

void SendUdpMsg(const struct sockaddr_in& server_addr,const void *msg,size_t n){
    int fd=Socket(AF_INET,SOCK_DGRAM,0);
    Sendto(fd,msg,n,0,(struct sockaddr *)&server_addr,sizeof(sockaddr));
}

#endif