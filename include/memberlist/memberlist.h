#ifndef _MEMBERLIST_H
#define _MEMBERLIST_H
//CURRENT DIR
#include "node.h"
#include "config.h"
//MY INCLUDE DIR
#include <misc/timer.hpp>
#include <misc/util.hpp>
#include <type/genmsg.h>
#include <mynet/net.h>
#include <mynet/wrapped.h>
//SYS INCLUDE PATH
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>

using namespace std;

// TCP listen 最大监听的连接数
#define LISTENNUM 256

// epoll 最大监听的事件
#define EPOLLSIZE 1024

//是否打印出收到连接对端的地址信息
#define PRINT_ADDRINFO

class memberlist
{
    friend class timer;

private:
    ostream logger;

    atomic<uint32_t> sequenceNum;      // Local sequence number
    uint32_t nextSeqNum(){ return sequenceNum.fetch_add(1);};
    uint32_t incarnation;      // Local incarnation number
    atomic<uint32_t> numNodes; // Number of known nodes (estimate)
    uint32_t pushPullReq;      // Number of push/pull requests

    config config; // Config of this member

    int tcpfd;                            // FD of the TCP Socket
    int udpfd;                            // FD of the UDP Socker
    int epollfd;                          // FD of the epoll I/O MUX
    struct epoll_event events[EPOLLSIZE]; // Struct for holding epoll_event when epoll_wait returns

    mutex nodeMutex;
    vector<NodeState *> nodes;           // Known nodes
    map<string, NodeState *> nodeMap;    // Maps Node.Name -> NodeState. It may be deleted in a later sequel
    map<string, NodeState *> nodeTimers; // Maps Node.Name -> suspicion timer
    vector<NodeState> kRandomNodes(uint8_t k,function<bool(NodeState *n)> exclude);

    void sendAndReceiveState(struct sockaddr_in& remote_node,bool join);

    void sendLocalState(struct sockaddr_in& remote_node,bool join);

    //when an epoll event happens
    void handleevent();
    //onReceive a new connection
    void handleconn(int connfd);
    //onReceive a tcp message
    void handletcp(int sockfd);
    //onReceive a udp message
    void handleudp(int sockfd);

    //onReceive a udp ping message
    void handlePing(MessageData &ping,sockaddr_in &remote_addr);

    //Begin schedule
    //do probe, state synchronization and gossip periodically
    bool scheduled;
    unique_ptr<timer> schedule_timer[3];
    void schedule();
    void deschedule();

    size_t probeIndex;
    void probe();
    void probenode(NodeState &node);
    void setprobepipes(uint32_t seqno,int ackpipe,int nackpipe,uint32_t probeinterval);

    mutex AckLock;
    map<uint32_t,AckHandler> AckHandlers;

    void pushpull();
    void gossip();

public:
    void newmemberlist();
    void clearmemberlist();
    memberlist(/* args */) : scheduled(false){
        sequenceNum.store(0);
    };
    ~memberlist();
    void join(const string& cluster_addr);

// some helper functino
#ifdef PRINT_ADDRINFO
    void printaddr(struct sockaddr_in &remote_addr)
    {
        char addr[INET_ADDRSTRLEN];
        uint16_t port;
        if (inet_ntop(remote_addr.sin_family, &remote_addr.sin_addr, addr, sizeof(addr)) <= 0)
        {
            exit(-1);
        }
        printf("Receive from %s:%u\n", addr, port);
    }
#endif
};

#endif