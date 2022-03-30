#ifndef _NET_H
#define _NET_H
#include "wrapped.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <chrono>
#include <stdlib.h>

void SendTcpMsg(const struct sockaddr_in &server_addr, const void *msg, size_t n)
{
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(fd, (struct sockaddr *)&server_addr, sizeof(sockaddr));
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

void SendUdpMsg(const struct sockaddr_in &server_addr, const void *msg, size_t n)
{
    int fd = Socket(AF_INET, SOCK_DGRAM, 0);
    Sendto(fd, msg, n, 0, (struct sockaddr *)&server_addr, sizeof(sockaddr));
}

#endif