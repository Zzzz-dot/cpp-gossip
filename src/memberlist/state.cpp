#include <memberlist/memberlist.h>

using namespace std;

// #define NODE_LOCK                \
//     if (shutdown.load() == true) \
//     {                            \
//         return;                  \
//     }                            \
//     lock_guard<mutex> l(nodeMutex);

#define NODE_LOCK lock_guard<mutex> l(nodeMutex);

// Perform a single round of failure detection and gossip
void memberlist::probe()
{
#ifdef DEBUG
    LOG(INFO) << "[DEBUG] Start probe()" << endl;
#endif
    size_t numCheck = 0;
START:
    {
        // if (shutdown.load() == true)
        // {
        //     return;
        // }
        unique_lock<mutex> l(nodeMutex);

        // Make sure we don't wrap around infinitely
        if (numCheck >= nodes.size())
        {
            l.unlock();
            return;
        }

        // Handle the wrap around case
        if (probeIndex >= nodes.size())
        {
            random_shuffle(nodes.begin(), nodes.end());
            l.unlock();
            probeIndex = 0;
            numCheck++;
            goto START;
        }

        // Determine if we should probe this node
        bool skip = false;
        nodeState node;

        node = *nodes[probeIndex];
        l.unlock();
        if (node.N.Name == config->Name)
        {
            skip = true;
        }
        else if (node.DeadOrLeft())
        {
            skip = true;
        }

        // Potentially skip
        probeIndex++;
        if (skip)
        {
            numCheck++;
            goto START;
        }
        // Probe the specific node
        probeNode(node);
    }
}

// Send a ping request to the target node and wait for reply
void memberlist::probeNode(nodeState &node)
{
    uint32_t seqno = nextSeqNum();
    auto ping = genPing(seqno, node.N.Name, config->AdvertiseAddr, config->AdvertisePort, config->Name);

    int ackfd[2];
    Pipe(ackfd);
    int nackfd[2];
    Pipe2(nackfd, O_DIRECT);

    // Set Probe Pipes
    setProbePipes(seqno, ackfd[1], nackfd[1], config->ProbeInterval);

    struct sockaddr_in addr = node.N.FullAddr();

    int64_t sendtime = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();

    int64_t deadline = sendtime + config->ProbeInterval;

    if (node.State == StateAlive)
    {
        encodeSendUDP(udpFd, &addr, ping);
    }
    // Else, this is a suspected node
    else
    {
        // This two messages con be composed into one compound message, but currently our implementation doesn't support compound message yet
        encodeSendUDP(udpFd, &addr, ping);
        auto suspect = genSuspectMsg(node.Incarnation, node.N.Name, config->Name);
        encodeSendUDP(udpFd, &addr, suspect);
    }

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(ackfd[0], &rset);
    struct timeval probetimeout;
    probetimeout.tv_usec = config->ProbeTimeout;

    Select(ackfd[0] + 1, &rset, nullptr, nullptr, &probetimeout);
    if (FD_ISSET(ackfd[0], &rset))
    {
        ackMessage ackmsg;
        Read(ackfd[0], (void *)&ackmsg, sizeof(ackMessage));
        if (ackmsg.Complete == true)
        {
            clearProbePipes(ackfd, nackfd);
            return;
        }
        // Some corner case when akcmsg.Complete==false
        else
        {
            Write(ackfd[1], (void *)&ackmsg, sizeof(ackMessage));
        }
    }
    // Probe Timeout
    else
    {
#ifdef DEBUG
        LOG(INFO) << "[DEBUG] memberlist: Failed UDP ping: " << node.N.Name << " (timeout reached)" << endl;
#endif
    }

    // IndirectProbe If True Ack is not received after ProbeTimeout
    vector<nodeState> kNodes;
    {
        NODE_LOCK
        kNodes = kRandomNodes(config->IndirectChecks, nodes, [this](shared_ptr<nodeState> n) -> bool
                              { return n->N.Name == this->config->Name || n->State != StateAlive; });
    }

    auto indirectping = genIndirectPing(seqno, node.N.Name, node.N.Addr, node.N.Port, true, config->AdvertiseAddr, config->AdvertisePort, config->Name);
    for (size_t i = 0; i < kNodes.size(); i++)
    {
        struct sockaddr_in addr = kNodes[i].N.FullAddr();
        encodeSendUDP(udpFd, &addr, indirectping);
    }

    // Also make an attempt to contact the node directly over TCP. This
    // helps prevent confused clients who get isolated from UDP traffic
    // but can still speak TCP (which also means they can possibly report
    // misinformation to other nodes via anti-entropy), avoiding flapping in
    // the cluster.
    int fallbackfd[2];
    Pipe(fallbackfd);
    if (config->AllowTcpPing)
    {
        thread([this, fallbackfd, &node, &ping, deadline]
               {
                    bool didConnect=this->sendPingAndWaitForAck(node.N.FullAddr(),ping,deadline);
                    Write(fallbackfd[1],(void*)&didConnect,sizeof(bool)); })
            .detach();
    }

    // Wait for IndirectPing
    ackMessage ackmsg;
    Read(ackfd[0], (void *)&ackmsg, sizeof(ackMessage));
    if (ackmsg.Complete == true)
    {
        clearProbePipes(ackfd, nackfd, fallbackfd);
        return;
    }

    bool didConnect;
    Read(fallbackfd[0], (void *)&didConnect, sizeof(bool));
    if (config->AllowTcpPing && didConnect)
    {
        LOG(WARNING) << "memberlist: Was able to connect to " << node.N.Name << " over TCP but UDP probes failed, network may be misconfigured" << endl;
        clearProbePipes(ackfd, nackfd, fallbackfd);
        return;
    }

    {
        lock_guard<mutex> l(this->ackLock);
        this->ackHandlers.erase(seqno);
    }
    clearProbePipes(ackfd, nackfd, fallbackfd);

    // This node may fail, gossip suspect
    LOG(INFO) << "memberlist: Suspect " << node.N.Name << " has failed, no acks received!" << endl;
    auto suspect = genSuspectBroadcast(node.Incarnation, node.N.Name, config->Name);
    suspectNode(suspect);
}

// clearAckHandler is used to close the pipe
// when received ackmsg (whatever Complete = true or false)
void memberlist::clearProbePipes(int ackPipe[2], int nackPipe[2], int fallbackPipe[2])
{
    if (fallbackPipe != nullptr)
    {
        Close(fallbackPipe[0]);
        Close(fallbackPipe[1]);
    }
    Close(ackPipe[0]);
    Close(ackPipe[1]);
    Close(nackPipe[0]);
    Close(nackPipe[1]);
}

// setprobepipes is used to attach the ackpipe to receive a message when an ack
// with a given sequence number is received.
void memberlist::setProbePipes(uint32_t seqNo, int ackPipe, int nackPipe, uint32_t probeInterval)
{
    // onRceive Ack Resp
    auto ackFn = [ackPipe](int64_t timestamp)
    {
        ackMessage ackmsg(true, timestamp);
        Write(ackPipe, (void *)&ackmsg, sizeof(ackMessage));
    };

    // onReceive Nack Resp
    auto nackFn = [nackPipe]()
    {
        Write(nackPipe, "", 1);
    };

    // Callback functino after prointerval, this onceTimer is invoked only when the Ack expired;
    auto f = [this, seqNo, ackPipe, nackPipe]
    {
        int64_t now = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        ackMessage ackmsg(false, now);
        Write(ackPipe, (void *)&ackmsg, sizeof(ackMessage));
    };

    // Add ackHandlers
    {
        onceTimer t = onceTimer(probeInterval, f, nullptr);
        lock_guard<mutex> l(ackLock);
        auto ackhand = make_shared<ackHandler>(ackFn, nackFn, t);
        ackHandlers[seqNo] = ackhand;
    }
    // Start onceTimer
    ackHandlers[seqNo]->t.Run();
}

// setAckHandler is used to attach a handler to be invoked when an ack with a
// given sequence number is received. If a timeout is reached, the handler is
// deleted. This is used for indirect pings so does not configure a function
// for nacks.
void memberlist::setAckHandler(uint32_t seqNo, function<void(int64_t timestamp)> ackFn, int64_t timeout)
{
    onceTimer t = onceTimer(timeout, nullptr, nullptr);
    lock_guard<mutex> l(ackLock);
    auto ackhand = make_shared<ackHandler>(ackFn, nullptr, t);
    ackHandlers[seqNo] = ackhand;
}

// refute gossips an alive message in response to incoming information that we
// are suspect or dead. It will make sure the incarnation number beats the given
// accusedInc value, or you can supply 0 to just get the next incarnation number.
// This alters the node state that's passed in so this MUST be called while the
// nodeLock is held.
void memberlist::refute(shared_ptr<nodeState> me, uint32_t accusedInc)
{
    uint32_t inc = incarnation.load();
    if (accusedInc >= inc)
    {
        inc = skipIncarnation(accusedInc - inc + 1);
    }
    me->Incarnation = inc;

    auto aliveMsg = genAliveBroadcast(inc, me->N.Name, me->N.Addr, me->N.Port);
    onlyBroadcast(me->N.Name, aliveMsg);
}

void memberlist::aliveNode(const Broadcast &b, bool bootstrap, int notifyfd)
{
    auto a = b.alive();

    NODE_LOCK
    shared_ptr<nodeState> state;
    bool updateNode = false;

    // It is possible that during a Leave(), there is already an aliveMsg
    // in-queue to be processed but blocked by the locks above. If we let
    // that aliveMsg process, it'll cause us to re-join the cluster. This
    // ensures that we don't.
    if (leave.load() && a.node() == config->Name)
    {
        return;
    }

    // Check if we've never seen this node before, and if not, then
    // store this node in our node map.
    if (nodeMap.find(a.node()) == nodeMap.end())
    {
        state = make_shared<nodeState>(Node(a.node(), a.addr(), a.port()), 0, StateDead);

        nodeMap[a.node()] = state;

        // Add this state to random offset. This is important to ensure
        // the failure detection bound is low on average. If all
        // nodes did an append, failure detection bound would be
        // very high.
        if (nodes.empty())
        {
            nodes.push_back(state);
        }
        else
        {
            size_t n = nodes.size();
            size_t offset = rand() % n;
            nodes.push_back(nodes[offset]);
            nodes[offset] = state;
        }

        numNodes.fetch_add(1);
    }
    else
    {
        state = nodeMap[a.node()];

        // Check if this address is different than the existing node unless the old node is dead
        if (a.addr() != state->N.Addr || a.port() != state->N.Port)
        {
            // If DeadNodeReclaimTime is configured, check if enough time has elapsed since the node died.
            int64_t now = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
            bool canReclaim = config->DeadNodeReclaimTime > 0 && now - state->StateChange >= config->DeadNodeReclaimTime;

            // Allow the address to be updated if a dead node is being replaced.
            if (state->State == StateLeft || (state->State == StateDead && canReclaim))
            {
                updateNode = true;
                LOG(INFO) << "memberlist: Updating address for left or failed node " << a.node() << " form " << state->N.Addr << ":" << state->N.Port << " to " << a.addr() << ":" << a.port() << endl;
            }
            else
            {
                LOG(ERROR) << "memberlist: Conflicting address for " << a.node() << " Mine: " << state->N.Addr << ":" << state->N.Port << " Theirs: " << a.addr() << ":" << a.port() << " Old State: " << state->State << endl;
                return;
            }
        }
    }

    bool isLocalNode = state->N.Name == config->Name;
    // Bail if strictly less and this is about us
    if (isLocalNode)
    {
        if (a.incarnation() < state->Incarnation)
        {
            return;
        }
    }
    // Bail if the incarnation number is older, and this is not about us
    else
    {
        if (a.incarnation() <= state->Incarnation && !updateNode)
        {
            return;
        }
    }

    // 1. isLocalNode && a.incarnation()>=state->Incarnation
    // 2. !isLocalNode && (a.incarnation()>state->Incarnation||updateNode)

    // Clear out any suspicion timer that may be in effect.
    nodeTimers.erase(a.node());

    // If this alive msg is received from other node, then bootstrap is false;
    // If this alive msg is generated by ourself to broadcast, then bootstrap is true.
    if (!bootstrap && isLocalNode)
    {

        // If the Incarnation is the same, we need special handling, since it
        // possible for the following situation to happen:
        // 1) Start with configuration C, join cluster
        // 2) Hard fail / Kill / Shutdown
        // 3) Restart with configuration C', join cluster
        //
        // In this case, other nodes and the local node see the same incarnation,
        // but the values may not be the same. For this reason, we always
        // need to do an equality check for this Incarnation. In most cases,
        // we just ignore, but we may need to refute.

        if (a.incarnation() == state->Incarnation)
        {
            return;
        }
        refute(state, a.incarnation());
        LOG(WARNING) << "memberlist: Refuting an alive message for '" << a.node() << "' (" << a.addr() << ":" << a.port() << ")" << endl;
    }
    else
    {
        auto aliveMsg = genAliveBroadcast(a);
        BroadcastNotify(a.node(), aliveMsg, notifyfd);

        // Update the state and incarnation number
        state->Incarnation = a.incarnation();
        state->N.Addr = a.addr();
        state->N.Port = a.port();
        if (state->State != StateAlive)
        {
            state->State = StateAlive;
            state->StateChange = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        }
    }
}

void memberlist::suspectNode(const Broadcast &b)
{
    auto s = b.suspect();
    NODE_LOCK

    // If we've never heard about this node before, ignore it
    if (nodeMap.find(s.node()) == nodeMap.end())
    {
        return;
    }

    shared_ptr<nodeState> state = nodeMap[s.node()];

    // Ignore old incarnation numbers
    if (s.incarnation() < state->Incarnation)
    {
        return;
    }

    // See if there's a suspicion timer we can confirm. If the info is new
    // to us we will go ahead and re-gossip it. This allows for multiple
    // independent confirmations to flow even when a node probes a node
    // that's already suspect.
    if (auto timer = nodeTimers.find(s.node()); timer != nodeTimers.end())
    {
        if (timer->second->Confirm(s.from()))
        {
            auto suspectMsg = genSuspectBroadcast(s);
            onlyBroadcast(s.node(), suspectMsg);
        }
        return;
    }

    // Ignore non-alive nodes
    if (state->State != StateAlive)
    {
        return;
    }

    // If this is us we need to refute, otherwise re-broadcast
    if (state->N.Name == config->Name)
    {
        refute(state, s.incarnation());
        LOG(WARNING) << "memberlist: Refuting a suspect message (from: " << s.from() << ")" << endl;
        return;
    }
    else
    {
        auto suspectMsg = genSuspectBroadcast(s);
        onlyBroadcast(s.node(), suspectMsg);
    }

    // Update the state
    state->Incarnation = s.incarnation();
    state->State = StateSuspect;
    int64_t now = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    state->StateChange = now;

    // Setup a suspicion timer. Given that we don't have any known phase
    // relationship with our peers, we set up k such that we hit the nominal
    // timeout two probe intervals short of what we expect given the suspicion
    // multiplier.
    uint8_t k = config->SuspicionMult - 2;

    // If there aren't enough nodes to give the expected confirmations, just
    // set k to 0 to say that we don't expect any. Note we subtract 2 from n
    // here to take out ourselves and the node being probed.
    uint32_t n = numNodes.load();
    if (n - 2 < k)
    {
        k = 0;
    }

    // Compute the timeouts based on the size of the cluster.
    int64_t min = suspicionTimeout(config->SuspicionMult, n, config->ProbeInterval);
    int64_t max = config->SuspicionMaxTimeoutMult * min;
    
    auto f = [this, s, now](suspicion *sup)
    {
        Broadcast d;
        bool timeout = false;
        {
            lock_guard<mutex> l(this->nodeMutex);
            auto state = this->nodeMap.find(s.node());

            timeout = state != this->nodeMap.end() && state->second->State == StateSuspect && state->second->StateChange == now;
            if (timeout)
            {
                d = genDeadBroadcast(state->second->Incarnation, state->second->N.Name, this->config->Name);
            }
        }
        if (timeout)
        {
            LOG(INFO) << "memberlist: Marking " << s.node() << " as failed, suspect timeout reached "
                      << "(" << sup->Load() << " peer confirmations)" << endl;
            //Fixme 删除自身
            this->deadNode(d);
        }
    };

    nodeTimers[s.node()] = make_shared<suspicion>(s.from(), k, min, max, f);
}

void memberlist::deadNode(const Broadcast &b)
{
    auto d = b.dead();
    NODE_LOCK

    // If we've never heard about this node before, ignore it
    if (nodeMap.find(d.node()) == nodeMap.end())
    {
        return;
    }

    shared_ptr<nodeState> state = nodeMap[d.node()];

    // Ignore old incarnation numbers
    if (d.incarnation() < state->Incarnation)
    {
        return;
    }

    // Clear out any suspicion timer that may be in effect.
    nodeTimers.erase(d.node());

    // Ignore if node is already dead
    if (state->DeadOrLeft())
    {
        return;
    }

    // Check if this is us
    if (state->N.Name == config->Name)
    {
        // If we are not leaving we need to refute
        if (leave.load() == false)
        {
            refute(state, d.incarnation());
            LOG(WARNING) << "memberlist: Refuting a dead message (from: " << d.from() << ")" << endl;
            return;
        }

        // If we are leaving, we broadcast and wait
        auto deadMsg = genDeadBroadcast(d);
        BroadcastNotify(d.node(), deadMsg, leaveFd[1]);
    }
    else
    {
        auto deadMsg = genDeadBroadcast(d);
        onlyBroadcast(d.node(), deadMsg);
    }

    // Update the state
    state->Incarnation = d.incarnation();

    // If the dead message was send by the node itself, mark it is left
    // instead of dead.
    if (d.node() == d.from())
    {
        state->State = StateLeft;
    }
    else
    {
        state->State = StateDead;
    }
    state->StateChange = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

// pushPull is invoked periodically to randomly perform a complete state
// exchange. Used to ensure a high level of convergence, but is also
// reasonably expensive as the entire state of this node is exchanged
// with the other node.
void memberlist::pushPull()
{
#ifdef DEBUG
    LOG(INFO) << "[DEBUG] Start pushPull()" << endl;
#endif
    vector<nodeState> v;
    {
        NODE_LOCK
        v = kRandomNodes(1, nodes, [this](shared_ptr<nodeState> n) -> bool
                         { return n->N.Name == this->config->Name || n->State != StateAlive; });
    }

    if (v.empty())
    {
        return;
    }

    nodeState n = v[0];
    try{
        pushPullNode(n.N.FullAddr(), false);
    }catch(wrapException){
        LOG(ERROR)<<"memberlist: cannot pushpull with the remote node"<<endl;
    }
}

// pushPullNode does a complete state exchange with a specific node.
void memberlist::pushPullNode(const sockaddr_in &remote_addr, bool join)
{
    auto md = sendAndReceiveState(remote_addr, join);

    if (md.head() == MessageData_MessageType_pushPullMsg)
    {
        auto pushpull = md.pushpull();
        mergeRemoteState(pushpull);
    }
}

// gossip is invoked every GossipInterval period to broadcast our gossip
// messages to a few random nodes.

void memberlist::gossip()
{
#ifdef DEBUG
    LOG(INFO) << "[DEBUG] Start gossip()" << endl;
#endif
    vector<nodeState> v;
    {
        NODE_LOCK
        auto exclude = [this](shared_ptr<nodeState> n) -> bool
        {
            if (n->N.Name == this->config->Name)
            {
                return true;
            }
            switch (n->State)
            {
            case StateAlive:
            case StateSuspect:
                return false;
                break;
            case StateDead:
            {
                int64_t ts = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
                return ts - n->StateChange >= this->config->GossipToTheDeadTime;
                break;
            }
            default:
                return true;
                break;
            }
        };
        v = kRandomNodes(config->GossipNodes, nodes, exclude);
    }

    size_t bytesAvail = config->UDPBufferSize;
    for (size_t i = 0; i < v.size(); i++)
    {

        ComBroadcast cbc;
        bool haveMsg = getBroadcasts(MessageDataOverhead, bytesAvail, cbc);
        if (!haveMsg)
        {
            return;
        }

        MessageData md = genComMsg(cbc);

        sockaddr_in remote_addr = v[i].N.FullAddr();

        encodeSendUDP(udpFd, &remote_addr, md);
    }
}

// sendPingAndWaitForAck makes a stream connection to the given address, sends
// a ping, and waits for an ack. All of this is done as a series of blocking
// operations, given the deadline. The bool return parameter is true if we
// we able to round trip a ping to the other node.
bool memberlist::sendPingAndWaitForAck(const sockaddr_in &a, MessageData &ping, int64_t deadline)
{
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    int64_t now;

    now = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    if (deadline - now <= 0)
    {
        return false;
    }
    ConnectTimeout(fd, (sockaddr *)&a, sizeof(sockaddr_in), deadline - now);

    now = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    if (deadline - now < 0)
    {
        return false;
    }
    struct timeval tcptimeout;
    tcptimeout.tv_usec = deadline - now;

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tcptimeout, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tcptimeout, sizeof(struct timeval));

    encodeSendTCP(fd, ping);

    auto resp = decodeReceiveTCP(fd);

    if (resp.head() != MessageData_MessageType_ackRespMsg)
    {
        LOG(ERROR) << "Unexpected msgType (" << resp.head() << ") from ping " << LogCoon(fd) << endl;
        Close(fd);
        return false;
    }

    auto ack = resp.ackresp();
    if (ack.seqno() != ping.ping().seqno())
    {
        LOG(ERROR) << "Sequence number from ack (" << ack.seqno() << ") doesn't match ping (" << ping.ping().seqno() << ")" << endl;
        Close(fd);
        return false;
    }

    Close(fd);
    return true;
}

// sendAndReceiveState is used to initiate a push/pull over a stream with a
// remote host.
MessageData memberlist::sendAndReceiveState(const sockaddr_in &remote_addr, bool join)
{

    // Connection Timeout
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    ConnectTimeout(fd, (struct sockaddr *)&remote_addr, sizeof(sockaddr), config->TCPTimeout);

    struct timeval tcptimeout;
    tcptimeout.tv_usec = config->TCPTimeout;

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tcptimeout, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tcptimeout, sizeof(struct timeval));

    sendLocalState(fd, join);

    auto pushpull = decodeReceiveTCP(fd);

    Close(fd);

    if (pushpull.head() == MessageData::MessageType::MessageData_MessageType_errMsg)
    {
        LOG(INFO) << "Remote error: " << pushpull.errresp().error() << endl;
        return MessageData();
    }

    if (pushpull.head() != MessageData::MessageType::MessageData_MessageType_pushPullMsg)
    {
        LOG(WARNING) << "Receive invalid msgType: " << pushpull.head() << ", expected: " << MessageData::MessageType::MessageData_MessageType_pushPullMsg << endl;
        return MessageData();
    }
    return pushpull;
}

// sendLocalState is invoked to send our local state over a stream connection.
void memberlist::sendLocalState(int fd, bool join)
{
    // This version of the implementation does not contain TCPTimeout
    auto pushpull = genPushPull(join);
    {
        NODE_LOCK
        for (size_t i = 0; i < nodes.size(); i++)
        {
            addPushNodeState(pushpull, nodes[i]->N.Name, nodes[i]->N.Addr, nodes[i]->N.Port, nodes[i]->Incarnation, PushNodeState::NodeStateType(nodes[i]->State));
        }
    }
    encodeSendTCP(fd, pushpull);
}

void memberlist::mergeRemoteState(const PushPull &pushpull)
{
    auto &states = pushpull.states();
    int size = pushpull.states_size();
    for (int i = 0; i < size; i++)
    {
        auto &state = states[i];
        Broadcast b;
        switch (state.state())
        {
        case PushNodeState_NodeStateType_StateAlive:
            b = genAliveBroadcast(state.incarnation(), state.name(), state.addr(), state.port());
            aliveNode(b, false, -1);
            break;
        case PushNodeState_NodeStateType_StateLeft:
            b = genDeadBroadcast(state.incarnation(), state.name(), state.name());
            deadNode(b);
            break;
        case PushNodeState_NodeStateType_StateDead:
            // If the remote node believes a node is dead, we prefer to
            // suspect that node instead of declaring it dead instantly
        case PushNodeState_NodeStateType_StateSuspect:
            b = genSuspectBroadcast(state.incarnation(), state.name(), config->Name);
            suspectNode(b);
            break;
        }
    }
}

// getNextMessage returns the next message to process in priority order, using LIFO
bool memberlist::getNextMessage(msgHandoff *msg)
{
    lock_guard<mutex> l(msgQueueMutex);
    if (!highPriorityMsgQueue.empty())
    {
        *msg = highPriorityMsgQueue.top();
        highPriorityMsgQueue.pop();
        return true;
    }

    if (!lowPriorityMsgQueue.empty())
    {
        *msg = lowPriorityMsgQueue.top();
        lowPriorityMsgQueue.pop();
        return true;
    }

    return false;
}