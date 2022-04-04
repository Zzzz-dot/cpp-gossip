#ifndef _MEMBERLIST_H
#define _MEMBERLIST_H
#include <misc/timer.hpp>
#include "node.h"
#include "type/genmsg.hpp"
#include <net/config.h>
#include <net/wrapped.h>

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

    void handlemsg();

    //Begin schedule
    //do probe, state synchronization and gossip periodically
    bool scheduled;
    unique_ptr<timer> schedule_timer[3];
    void schedule();
    void deschedule();

    size_t probeIndex;
    void probe();
    void probenode(NodeState *node);
    void pushpull();
    void gossip();

public:
    memberlist(/* args */) : scheduled(false){
        sequenceNum.store(0);
    };
    ~memberlist();

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

void memberlist::handlemsg()
{
    auto handletcp = [](int coonfd)
    {
        Epoll_ctl(epollfd, ) for (;;)
        {
        }
    };

    auto handleevent = [this]()
    {
        for (;;)
        {
            int events_num = Epoll_wait(this->epollfd, this->events, EPOLLSIZE, -1);
            for (int i = 0; i < events_num; i++)
            {
                int socketfd = this->events[i].data.fd;
                struct sockaddr_in remote_addr;
                bzero(&remote_addr, sizeof(sockaddr_in));
                socklen_t socklen = sizeof(sockaddr_in);

                // Receive a new connection from TCP
                if (socketfd == this->tcpfd)
                {
                    int connfd = Accept(socketfd, (struct sockaddr *)&remote_addr, &socklen);
                    handlemsg(connfd);
                }
                // Receive a new message from UDP
                else if (socketfd == this->udpfd)
                {
                }
                // Receive a new message from TCP
                else
                {
                }
#ifdef PRINT_ADDRINFO
                printaddr(remote_addr);
#endif
            }
        }
    }
}

memberlist::memberlist(/* args */)
{
    // Init local address
    struct sockaddr_in local_addr;
    bzero(&local_addr, sizeof(sockaddr_in));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.BindPort);
    if (int e = inet_pton(AF_INET, config.BindAddr.c_str(), &local_addr.sin_addr) <= 0)
    {
        errno = e;
    }

    // Create TCP socket
    tcpfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(tcpfd, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    Listen(tcpfd, LISTENNUM);
    // Create UDP socket
    udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(udpfd, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    // Create epoll
    epollfd = Epoll_create(EPOLLSIZE);

    // Add the tcpfd and udpfd to the watchlist of epollfd
    struct epoll_event ev1;
    ev1.events = EPOLLIN;
    ev1.data.fd = tcpfd;
    Epoll_ctl(epollfd, EPOLL_CTL_ADD, tcpfd, &ev1);
    struct epoll_event ev2;
    ev2.events = EPOLLIN;
    ev2.data.fd = udpfd;
    Epoll_ctl(epollfd, EPOLL_CTL_ADD, tcpfd, &ev2);
}

memberlist::~memberlist()
{
}

// Schedule is used to ensure the timer is performed periodically. This
// function is safe to call multiple times. If the memberlist is already
// scheduled, then it won't do anything.
void memberlist::schedule()
{
    if (scheduled)
        return;
    else
        scheduled = true;

    // probe,pushpull,gossip这几个函数可能需要转换成公有

    // Probe
    if (config.ProbeInterval > 0)
    {
        schedule_timer[0] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::probe, this), this));
    }

    // PushPull
    if (config.PushPullInterval > 0)
    {
        schedule_timer[1] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::pushpull, this), this, true));
    }

    // Gossip
    if (config.GossipInterval > 0)
    {
        schedule_timer[2] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::gossip, this), this));
    }
}

// Deschedule is used to stop the background maintenance. This is safe
// to call multiple times.
void memberlist::deschedule()
{
    if (!scheduled)
        return;
    else
        scheduled = false;

    schedule_timer[0].release();
    schedule_timer[1].release();
    schedule_timer[2].release();
}

// Perform a single round of failure detection and gossip
void memberlist::probe()
{
    size_t numCheck = 0;
START:
    {
        unique_lock<mutex> l(nodeMutex);
        // Make sure we don't wrap around infinitely
        if (numCheck >= nodes.size())
        {
            l.unlock();
            return;
        }

        // Handle the wrap around case
	    if (probeIndex >= nodes.size()) {
            random_shuffle(nodes.begin(),nodes.end());
		    l.unlock();
		    probeIndex = 0;
		    numCheck++;
		    goto START;
	    }

        // Determine if we should probe this node
	    bool skip = false;
	    NodeState node; 

	    node = *nodes[probeIndex];
        l.unlock();
	    if (node.Node.Name == config.Name) {
		    skip = true;
	    } else if (node.DeadOrLeft()) {
		    skip = true;
	    }

	    // Potentially skip
	    probeIndex++;
	    if (skip) {
		    numCheck++;
		    goto START;
	    }
        // Probe the specific node
	    probenode(&node);
    }
}

// Send a ping request to the target node and wait for reply
void memberlist::probenode(NodeState *node)
{
    auto ping=genPing(nextSeqNum(),node->Node.Name,config.BindAddr,config.BindPort,config.Name);

    //匿名管道
    int ackfd[2];
    Pipe2(ackfd,O_DIRECT);
    int nackfd[2];
    Pipe2(nackfd,O_DIRECT);
    




}



}

#endif