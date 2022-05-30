#ifndef _MEMBERLIST_H
#define _MEMBERLIST_H

// MY INCLUDE DIR
#include <cmakeConfig.h>

#include <misc/node.h>
#include <misc/config.h>
#include <misc/suspicion.h>
#include <misc/broadcastQueue.h>
#include <misc/timer.h>
#include <misc/util.h>

#include <type/genmsg.h>
#include <type/msgtype.pb.h>

#include <mynet/broadcast.h>
#include <mynet/net.h>
#include <mynet/wrapped.h>

// SYS INCLUDE PATH
#include <glog/logging.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
//#include <string.h>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>
#include <stack>
#include <thread>

#define LISTENNUM 1024

class memberlist
{
    friend class timer;

private:
    std::atomic<uint32_t> sequenceNum; // Local sequence number
    uint32_t nextSeqNum();
    std::atomic<uint32_t> incarnation; // Local incarnation number
    uint32_t nextIncarnation();
    uint32_t skipIncarnation(uint32_t offset);

    std::atomic<uint32_t> numNodes;    // Number of known nodes (estimate)
    std::atomic<uint32_t> pushPullReq; // Number of push/pull requests

    std::shared_ptr<Config> config; // Config of this member
    std::atomic<bool> leave;
    int leaveFd[2];
    std::atomic<bool> shutdown;
    int shutdownFd[2];

    int tcpFd; // FD of the TCP Socket
    int udpFd; // FD of the UDP Socker

    int handoffFd[2];
    std::mutex msgQueueMutex;
    struct msgHandoff
    {
        Broadcast b;
        struct sockaddr_in from;
        msgHandoff() = default;
        msgHandoff(const Broadcast &b_, const struct sockaddr_in &from_);
    };
    std::stack<msgHandoff> highPriorityMsgQueue;
    std::stack<msgHandoff> lowPriorityMsgQueue;
    bool getNextMessage(msgHandoff *msg);

    std::mutex nodeMutex;
    std::vector<std::shared_ptr<nodeState>> nodes;           // Known nodes
    std::map<std::string, std::shared_ptr<nodeState>> nodeMap;    // Maps Node.Name -> nodeState. It may be deleted in a later sequel
    std::map<std::string, std::shared_ptr<suspicion>> nodeTimers; // Maps Node.Name -> suspicion timer

    void refute(std::shared_ptr<nodeState> me, uint32_t accusedInc);
    void aliveNode(const Broadcast &b, bool bootstrap, int notifyfd);
    void suspectNode(const Broadcast &b);
    void deadNode(const Broadcast &b);

    std::shared_ptr<broadcastQueue> TransmitLimitedQueue;
    void onlyBroadcast(std::string node, const Broadcast &bc);
    void BroadcastNotify(std::string node, const Broadcast &bc, int notifyFd);
    bool getBroadcasts(size_t overhead, size_t limit, ComBroadcast &cbc);

    void streamListen();
    void packetListen();
    void packetHandler();

    void handleConn(int connfd);
    void handleCommand(MessageData &md, struct sockaddr_in &remote_addr, int64_t ts);

    void handlePushPull(const PushPull &pushpull, int connfd);
    void handlePing(const Ping &ping, int connfd);
    void handleUser(const User &u, int connfd);

    void handlePing(const Ping &ping, sockaddr_in &remote_addr);
    void handleIndirectPing(const IndirectPing &indirectPing, sockaddr_in &remote_addr);
    void handleAck(const AckResp &ack, sockaddr_in &remote_addr, int64_t ts);
    void handleNack(const NackResp &nack, sockaddr_in &remote_addr);
    void handleUser(const User &b, sockaddr_in &remote_addr);
    void handleComBroadcast(const ComBroadcast &comBroadcast, sockaddr_in &remote_addr);

    inline void handleAlive(const Broadcast &b, sockaddr_in &remote_addr);
    inline void handleDead(const Broadcast &b, sockaddr_in &remote_addr);
    inline void handleSuspect(const Broadcast &b, sockaddr_in &remote_addr);

    // Begin schedule
    // do probe, state synchronization and gossip periodically
    std::mutex tickerLock;
    bool scheduled;
    std::unique_ptr<repeatTimer> schedule_timer[3];
    void schedule();
    void deschedule();

    size_t probeIndex;
    void probe();
    void probeNode(nodeState &node);
    void setProbePipes(uint32_t seqNo, int ackPipe[2], int nackPipe[2], uint32_t probeInterval);

    std::mutex ackLock;
    std::map<uint32_t, std::shared_ptr<ackHandler>> ackHandlers;
    void setAckHandler(uint32_t seqNo, std::function<void(int64_t timestamp)> ackFn, int64_t timeout);

    void pushPull();
    void pushPullNode(const sockaddr_in &remote_addr, bool join);
    MessageData sendAndReceiveState(const sockaddr_in &remote_addr, bool join);
    void sendLocalState(int fd, bool join);
    void mergeRemoteState(const PushPull &pushpull);

    void gossip();

    void setAlive();
    void newmemberlist(std::shared_ptr<Config> config_);
    void clearmemberlist();

    bool anyAlive();

    std::thread t1, t2, t3;

public:
    memberlist();
    memberlist(std::shared_ptr<Config> config_);
    ~memberlist();

    void Join(const std::string &cluster_addr);
    void Leave(int64_t timeout);
    void ShutDown();
    void UpdateNode(int64_t timeout);

    Node *LocalNode();
    std::vector<Node *> Members();
    size_t NumMembers();
    uint32_t EstNumNodes();

    void SendBestEffort(std::shared_ptr<Node> to, std::string msg);
    void SendReliable(std::shared_ptr<Node> to, std::string msg);
};

#endif