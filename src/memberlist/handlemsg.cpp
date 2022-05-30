#include <memberlist/memberlist.h>

using namespace std;

// streamListen is a process that runs continuously in the background
// accepting TCP connections and handing them over to handleConn
void memberlist::streamListen()
{
    fd_set rset;
    FD_ZERO(&rset);
    int maxfd = max(shutdownFd[0], tcpFd);
    for (;;)
    {
        FD_SET(shutdownFd[0], &rset);
        FD_SET(tcpFd, &rset);
        Select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
        char buf[1];
        if (FD_ISSET(shutdownFd[0], &rset))
        {
            Read(shutdownFd[0], buf, sizeof(buf));
            return;
        }
        if (FD_ISSET(tcpFd, &rset))
        {
            struct sockaddr_in remote_addr;
            bzero(&remote_addr, sizeof(sockaddr_in));
            socklen_t socklen = sizeof(sockaddr_in);

            int connfd = Accept(tcpFd, (struct sockaddr *)&remote_addr, &socklen);

#ifdef DEBUG
            LOG(INFO) << "memberlist: Stream connection from " << LogAddr(remote_addr) << endl;
#endif
            auto t = thread(&memberlist::handleConn, this, connfd);
            t.detach();
        }
    }
}

// packetListen is a process that runs continuously in the background
// accepting UDP messages for handleCommand to process
void memberlist::packetListen()
{
    fd_set rset;
    FD_ZERO(&rset);
    int maxfd = max(shutdownFd[0], udpFd);
    for (;;)
    {
        FD_SET(shutdownFd[0], &rset);
        FD_SET(udpFd, &rset);
        Select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
        if (FD_ISSET(shutdownFd[0], &rset))
        {
            char buf[1];
            Read(shutdownFd[0], buf, sizeof(buf));
            return;
        }
        if (FD_ISSET(udpFd, &rset))
        {
            struct sockaddr_in remote_addr;
            bzero(&remote_addr, sizeof(sockaddr_in));
            socklen_t socklen = sizeof(sockaddr_in);

            auto md = decodeReceiveUDP(udpFd, remote_addr, socklen);

#ifdef DEBUG
            LOG(INFO) << "memberlist: Receive UDP message from " << LogAddr(remote_addr) << endl;
            LOG(INFO) << md.DebugString();
#endif

            int64_t ts = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();

            handleCommand(md, remote_addr, ts);
        }
    }
}

// packetHandler is a process that runs continuously in the background
// it extracts the gossip message from the message queue and processes it
void memberlist::packetHandler()
{
    fd_set rset;
    FD_ZERO(&rset);
    int maxfd = max(shutdownFd[0], handoffFd[0]);
    for (;;)
    {
        FD_SET(shutdownFd[0], &rset);
        FD_SET(handoffFd[0], &rset);
        Select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
        char buf[1024];
        if (FD_ISSET(shutdownFd[0], &rset))
        {
            Read(shutdownFd[0], buf, sizeof(buf));
            return;
        }
        if (FD_ISSET(handoffFd[0], &rset))
        {
            Read(handoffFd[0], buf, sizeof(buf));
            for (;;)
            {
                msgHandoff msg;
                bool ok = getNextMessage(&msg);
                if (!ok)
                {
                    break;
                }
                Broadcast bc = msg.b;
                struct sockaddr_in from = msg.from;
                switch (bc.type())
                {
                case Broadcast_BroadcastType_aliveMsg:
                    handleAlive(bc, from);
                    break;
                case Broadcast_BroadcastType_deadMsg:
                    handleDead(bc, from);
                    break;
                case Broadcast_BroadcastType_suspectMsg:
                    handleSuspect(bc, from);
                    break;
                default:
                    LOG(ERROR) << " memberlist: Message type not supported (packet handler)" << endl;
                    break;
                }
            }
        }
    }
}

// handleConn handles a single incoming stream connection from the transport.
// 1. PushPull MessageData
// 2. TCP Ping MessageData
// 3. TCP User MessageData
void memberlist::handleConn(int connfd)
{

    struct timeval tcptimeout;
    tcptimeout.tv_usec = config->TCPTimeout;

    // set read and write timeout
    setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tcptimeout, sizeof(struct timeval));
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tcptimeout, sizeof(struct timeval));

    auto md = decodeReceiveTCP(connfd);

#ifdef DEBUG
    LOG(INFO) << "memberlist: Receive TCP message from " << LogCoon(connfd) << endl;
    LOG(INFO) << md.DebugString();
#endif

    switch (md.head())
    {
    case MessageData_MessageType_pushPullMsg:
        handlePushPull(md.pushpull(), connfd);
        break;
    case MessageData_MessageType_pingMsg:
        handlePing(md.ping(), connfd);
        break;
    case MessageData_MessageType_userMsg:
        handleUser(md.user(), connfd);
        break;
    default:
        LOG(ERROR) << "memberlist: Received invalid msgType form " << LogCoon(connfd) << endl;
        break;
    }
}

// handleCommand processes the following UDP messages:
// 1. UDP Ping MessageData
// 2. IndirectPing MessageData
// 3. AckResp MessageData
// 4. NackResp MessageData
// The following messages (ComBroadcast) are placed in the message queue:
// 1. Alive Broadcast
// 2. Dead Broadcast
// 3. Suspect Broadcast
// 4. User Broadcast
void memberlist::handleCommand(MessageData &md, struct sockaddr_in &remote_addr, int64_t ts)
{
    switch (md.head())
    {
    case MessageData_MessageType_pingMsg:
        handlePing(md.ping(), remote_addr);
        break;
    case MessageData_MessageType_indirectPingMsg:
        handleIndirectPing(md.indirectping(), remote_addr);
        break;
    case MessageData_MessageType_ackRespMsg:
        handleAck(md.ackresp(), remote_addr, ts);
        break;
    case MessageData_MessageType_nackRespMsg:
        handleNack(md.nackresp(), remote_addr);
        break;
    case MessageData_MessageType_userMsg:
        handleUser(md.user(), remote_addr);
        break;
    case MessageData_MessageType_compoundBroad:
        handleComBroadcast(md.combroadcast(), remote_addr);
    default:
        LOG(ERROR) << "memberlist: Received invalid msgType form " << LogAddr(remote_addr) << endl;
        break;
    }
}

void memberlist::handlePushPull(const PushPull &pushpull, int connfd)
{
    pushPullReq.fetch_add(1);
    bool join = pushpull.join();
    sendLocalState(connfd, join);
    mergeRemoteState(pushpull);
    pushPullReq.fetch_sub(1);
}

void memberlist::handlePing(const Ping &ping, int connfd)
{
    if (ping.node() != "" && ping.node() != config->Name)
    {
        LOG(WARNING) << "memberlist: Got ping for unexpected node " << ping.node() << " from " << LogCoon(connfd) << endl;
    }

    auto ackmsg = genAckResp(ping.seqno());

    encodeSendTCP(connfd, ackmsg);
}

void memberlist::handlePing(const Ping &ping, sockaddr_in &remote_addr)
{
    string addr;
    if (ping.sourceaddr() != "")
    {
        addr = ping.sourceaddr();
    }
    else
    {
        char addr_[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remote_addr.sin_addr, addr_, sizeof(addr_));
        addr = string(addr_);
    }
    if (ping.node() != "" && ping.node() != config->Name)
    {
        LOG(WARNING) << "memberlist: Got ping for unexpected node " << ping.node() << " from " << LogAddr(remote_addr) << endl;
    }

    auto ackmsg = genAckResp(ping.seqno());

    encodeSendUDP(udpFd, &remote_addr, ackmsg);
}

void memberlist::handleIndirectPing(const IndirectPing &indirectPing, sockaddr_in &remote_addr)
{
    uint32_t localSeqNo = nextSeqNum();
    auto ping = genPing(localSeqNo, indirectPing.node(), config->AdvertiseAddr, config->AdvertisePort, config->Name);

    // Setup a response handler to relay the ack
    int cancelFd[2];
    Pipe(cancelFd);
    auto respHandler = [this, cancelFd, remote_addr, indirectPing](int64_t timestamp)
    {
        // Try to prevent the nack if we've caught it in time.
        Close(cancelFd[1]);

        auto ack = genAckResp(indirectPing.seqno());
        encodeSendUDP(this->udpFd, &remote_addr, ack);
    };
    setAckHandler(localSeqNo, respHandler, config->ProbeTimeout);

    struct sockaddr_in target_addr;
    bzero(&remote_addr, sizeof(sockaddr_in));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = indirectPing.targetport();
    if (int e = inet_pton(AF_INET, indirectPing.targetaddr().c_str(), &target_addr.sin_addr) <= 0)
    {
        errno = e;
    }

    // Send the ping.
    encodeSendUDP(udpFd, &target_addr, ping);

    // Setup a timer to fire off a nack if no ack is seen in time.
    if (indirectPing.nack())
    {
        auto f = [this, cancelFd, remote_addr, indirectPing]
        {
            fd_set rset;
            FD_ZERO(&rset);
            FD_SET(cancelFd[0], &rset);
            struct timeval t;
            t.tv_usec = this->config->ProbeTimeout;
            Select(cancelFd[0] + 1, &rset, nullptr, nullptr, &t);
            if (FD_ISSET(cancelFd[0], &rset))
            {
                Close(cancelFd[0]);
                return;
            }
            else
            {
                Close(cancelFd[0]);
                Close(cancelFd[1]);
                auto nack = genNackResp(indirectPing.seqno());
                encodeSendUDP(udpFd, &remote_addr, nack);
            }
        };

        auto t = thread(f);
        t.detach();
    }
}

void memberlist::handleAck(const AckResp &ack, sockaddr_in &remote_addr, int64_t ts)
{

    //fix me
    shared_ptr<ackHandler> ah;
    {
        lock_guard<mutex> l(ackLock);
        auto ackHandler = ackHandlers.find(ack.seqno());
        if (ackHandler == ackHandlers.end())
        {
            return;
        }
        ah = ackHandler->second;
        ackHandlers.erase(ackHandler);
    }
    ah->t.Stop();
    ah->ackFn(ts);
}

void memberlist::handleNack(const NackResp &nack, sockaddr_in &remote_addr)
{
    shared_ptr<ackHandler> ah;
    {
        lock_guard<mutex> l(ackLock);
        auto ackHandler = ackHandlers.find(nack.seqno());
        if (ackHandler == ackHandlers.end())
        {
            return;
        }
        ah = ackHandler->second;
    }
    if (ah->nackFn != nullptr)
    {
        ah->nackFn();
    }
}

void memberlist::handleComBroadcast(const ComBroadcast &comBroadcast, sockaddr_in &remote_addr)
{
    lock_guard<mutex> l(msgQueueMutex);
    for (int i = 0; i < comBroadcast.bs_size(); i++)
    {
        const Broadcast bc = comBroadcast.bs(i);
        if (bc.type() == Broadcast_BroadcastType_aliveMsg)
        {
            if (highPriorityMsgQueue.size() < config->HandoffQueueDepth)
            {
                highPriorityMsgQueue.emplace(bc, remote_addr);
                Write(handoffFd[1], "", 1);
            }
            else
            {
                LOG(WARNING) << "memberlist: high priority handler queue full, dropping message (aliveMsg) form " << LogAddr(remote_addr) << endl;
            }
        }
        else
        {
            if (lowPriorityMsgQueue.size() < config->HandoffQueueDepth)
            {
                lowPriorityMsgQueue.emplace(bc, remote_addr);
                Write(handoffFd[1], "", 1);
            }
            else
            {
                LOG(WARNING) << "memberlist: low priority handler queue full, dropping message (aliveMsg) form " << LogAddr(remote_addr) << endl;
            }
        }
    }
}

void memberlist::handleAlive(const Broadcast &a, sockaddr_in &remote_addr)
{
    if (a.type() != Broadcast_BroadcastType_aliveMsg)
    {
        return;
    }
    aliveNode(a, false, -1);
}

void memberlist::handleDead(const Broadcast &d, sockaddr_in &remote_addr)
{
    if (d.type() != Broadcast_BroadcastType_deadMsg)
    {
        return;
    }
    deadNode(d);
}

void memberlist::handleSuspect(const Broadcast &s, sockaddr_in &remote_addr)
{
    if (s.type() != Broadcast_BroadcastType_suspectMsg)
    {
        return;
    }
    suspectNode(s);
}

void memberlist::handleUser(const User &u, int connfd)
{
    return;
}

void memberlist::handleUser(const User &u, sockaddr_in &remote_addr)
{
    return;
}