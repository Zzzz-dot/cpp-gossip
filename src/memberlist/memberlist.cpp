#include <memberlist/memberlist.h>

using namespace std;

uint32_t memberlist::nextSeqNum()
{
    sequenceNum.fetch_add(1);
    return sequenceNum.load();
};

uint32_t memberlist::nextIncarnation()
{
    incarnation.fetch_add(1);
    return incarnation.load();
};

uint32_t memberlist::skipIncarnation(uint32_t offset)
{
    incarnation.fetch_add(offset);
    return incarnation.load();
};

// setAlive is used to mark this node as being alive. This is the same
// as if we received an alive notification our own network channel for
// ourself.

void memberlist::setAlive()
{
    auto aliveMsg = genAliveBroadcast(nextIncarnation(), config->Name, config->AdvertiseAddr, config->AdvertisePort);
    aliveNode(aliveMsg, true, -1);
}

memberlist::msgHandoff::msgHandoff(const Broadcast &b_, const struct sockaddr_in &from_) : b(b_), from(from_){};

memberlist::memberlist()
{

    auto config_ = DefaultLANConfig();

    LOG(INFO) << "cpp-gossip v" << PROJECT_VERSION << endl;

    newmemberlist(config_);

    setAlive();

    schedule();
}

memberlist::memberlist(shared_ptr<Config> config_)
{
    LOG(INFO) << "cpp-gossip v" << PROJECT_VERSION << endl;

    newmemberlist(config_);

    setAlive();

    schedule();
}

memberlist::~memberlist()
{
    Leave(1000000);
    ShutDown();
    clearmemberlist();
    if (t1.joinable())
    {
        t1.join();
    }
    if (t2.joinable())
    {
        t2.join();
    }
    if (t3.joinable())
    {
        t3.join();
    }
}

// newMemberlist creates the network listeners.
// Does not schedule execution of background maintenance.
void memberlist::newmemberlist(shared_ptr<Config> config_)
{
    sequenceNum.store(0);
    incarnation.store(0);
    numNodes.store(0);
    pushPullReq.store(0);

    config = config_;

    leave.store(false);
    shutdown.store(false);
    Pipe(leaveFd);
    Pipe(shutdownFd);

    // Init local address
    struct sockaddr_in local_addr;
    bzero(&local_addr, sizeof(sockaddr_in));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config->BindPort);
    if (int e = inet_pton(AF_INET, config->BindAddr.c_str(), &local_addr.sin_addr) <= 0)
    {
        errno = e;
    }

    // Create TCP socket
    tcpFd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(tcpFd, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    Listen(tcpFd, LISTENNUM);
    // Create UDP socket
    udpFd = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(udpFd, (struct sockaddr *)&local_addr, sizeof(sockaddr));

    Pipe2(handoffFd, O_DIRECT);

    auto f = bind(&memberlist::EstNumNodes, this);
    TransmitLimitedQueue = make_shared<broadcastQueue>(config->RetransmitMult, f);

    scheduled = false;

    probeIndex = 0;

    t1 = thread(&memberlist::streamListen, this);
    t2 = thread(&memberlist::packetListen, this);
    t3 = thread(&memberlist::packetHandler, this);
};

void memberlist::clearmemberlist()
{
    sequenceNum.store(0);
    incarnation.store(0);
    numNodes.store(0);
    pushPullReq.store(0);

    tcpFd = -1;
    udpFd = -1;

    {
        lock_guard<mutex> l(msgQueueMutex);
        Close(handoffFd[0]);
        Close(handoffFd[1]);
        highPriorityMsgQueue = stack<msgHandoff>();
        lowPriorityMsgQueue = stack<msgHandoff>();
    }

    {
        lock_guard<mutex> l(nodeMutex);
        nodes.clear();
        nodeMap.clear();
        nodeTimers.clear();
    }

    probeIndex = 0;

    {
        lock_guard<mutex> l(ackLock);
        ackHandlers.clear();
    }
}

// Join is used to take an existing Memberlist and attempt to join a cluster
// by contacting the given host and performing a state sync. Initially,
// the Memberlist only contains our own state, so doing this will cause
// remote node to become aware of the existence of this node, effectively
// joining the cluster.
void memberlist::Join(const string &cluster_addr)
{
    sockaddr_in remote_addr = resolveAddr(cluster_addr);

    pushPullNode(remote_addr, true);
}

// Leave will broadcast a leave message but will not shutdown the background
// listeners, meaning the node will continue participating in gossip and state
// updates.
//
// This will block until the leave message is successfully broadcasted to
// a member of the cluster, if any exist or until a specified timeout
// is reached.
//
// This method is safe to call multiple times, but must not be called
// after the cluster is already shut down.
void memberlist::Leave(int64_t timeout)
{
    if (shutdown.load())
    {
        LOG(WARNING) << "Leave after Shutdown" << endl;
    }

    if (leave.load() == false)
    {
        leave.store(true);
        shared_ptr<nodeState> state;
        {
            lock_guard<mutex> l(nodeMutex);
            if (nodeMap.find(config->Name) == nodeMap.end())
            {
                LOG(WARNING) << "memberlist: Leave but we're not in the node map" << endl;
                return;
            }
            else
            {
                state = nodeMap[config->Name];
            }
        }

        // This dead message is special, because Node and From are the
        // same. This helps other nodes figure out that a node left
        // intentionally. When Node equals From, other nodes know for
        // sure this node is gone.

        auto deadMsg = genDeadBroadcast(state->Incarnation, state->N.Name, state->N.Name);

        deadNode(deadMsg);

        // Block until the broadcast goes out
        if (anyAlive())
        {
            fd_set rset;
            FD_ZERO(&rset);
            int maxfd = leaveFd[0];
            FD_SET(leaveFd[0], &rset);
            int n;
            if (timeout > 0)
            {
                timeval t;
                t.tv_usec = timeout;
                n = Select(maxfd + 1, &rset, nullptr, nullptr, &t);
            }
            else
            {
                n = Select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
            }
            if (FD_ISSET(leaveFd[0], &rset))
            {
                char buf[1];
                Read(leaveFd[0], buf, 1);
            }
            else
            {
                LOG(ERROR) << "timeout waiting for leave broadcast" << endl;
            }
        }
    }
}

// Shutdown will stop any background maintenance of network activity
// for this memberlist, causing it to appear "dead". A leave message
// will not be broadcasted prior, so the cluster being left will have
// to detect this node's shutdown using probing. If you wish to more
// gracefully exit the cluster, call Leave prior to shutting down.
//
// This method is safe to call multiple times.
void memberlist::ShutDown()
{
    lock_guard<mutex> l(nodeMutex);
    if (shutdown.load())
    {
        return;
    }

    Close(tcpFd);

    shutdown.store(true);

    Close(shutdownFd[1]);

    deschedule();
}

// UpdateNode is used to trigger re-advertising the local node. This is
// primarily used with a Delegate to support dynamic updates to the local
// meta data.  This will block until the update message is successfully
// broadcasted to a member of the cluster, if any exist or until a specified
// timeout is reached.
void memberlist::UpdateNode(int64_t timeout)
{
    shared_ptr<nodeState> state;

    // Get the existing node
    {
        lock_guard<mutex> l(nodeMutex);
        state = nodeMap[config->Name];
    }

    auto aliveMsg = genAliveBroadcast(nextIncarnation(), config->Name, state->N.Addr, state->N.Port);

    int notifyFd[2];
    Pipe(notifyFd);

    aliveNode(aliveMsg, false, notifyFd[1]);

    if (anyAlive())
    {
        fd_set rset;
        FD_ZERO(&rset);
        int maxfd = notifyFd[0];
        FD_SET(notifyFd[0], &rset);
        int n;
        if (timeout > 0)
        {
            timeval t;
            t.tv_usec = timeout;
            n = Select(maxfd + 1, &rset, nullptr, nullptr, &t);
        }
        else
        {
            n = Select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
        }
        if (FD_ISSET(notifyFd[0], &rset))
        {
            char buf[1];
            Read(notifyFd[0], buf, 1);
        }
        else
        {
            LOG(ERROR) << "timeout waiting for update broadcast" << endl;
        }
    }
}

// Schedule is used to ensure the timer is performed periodically. This
// function is safe to call multiple times. If the memberlist is already
// scheduled, then it won't do anything.
void memberlist::schedule()
{
    lock_guard<mutex> l(tickerLock);
    if (scheduled)
        return;
    else
        scheduled = true;

    // probe,pushpull,gossip这几个函数可能需要转换成公有

    // Probe
    if (config->ProbeInterval > 0)
    {
        schedule_timer[0] = unique_ptr<repeatTimer>(new repeatTimer(config->ProbeInterval, bind(&memberlist::probe, this), nullptr));
        schedule_timer[0]->Run();
    }

    // PushPull
    if (config->PushPullInterval > 0)
    {
        auto f = bind(&memberlist::EstNumNodes, this);
        schedule_timer[1] = unique_ptr<repeatTimer>(new repeatTimer(config->PushPullInterval, bind(&memberlist::pushPull, this), f, true));
        schedule_timer[1]->Run();
    }

    // Gossip
    if (config->GossipInterval > 0)
    {
        schedule_timer[2] = unique_ptr<repeatTimer>(new repeatTimer(config->GossipInterval, bind(&memberlist::gossip, this), nullptr));
        schedule_timer[2]->Run();
    }
}

// Deschedule is used to stop the background maintenance. This is safe
// to call multiple times.
void memberlist::deschedule()
{
    lock_guard<mutex> l(tickerLock);
    if (!scheduled)
        return;
    else
        scheduled = false;

    schedule_timer[0].release();
    schedule_timer[1].release();
    schedule_timer[2].release();
}

bool memberlist::anyAlive()
{
    lock_guard<mutex> l(nodeMutex);
    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (!nodes[i]->DeadOrLeft() && nodes[i]->N.Name != config->Name)
        {
            return true;
        }
    }
    return false;
}

// LocalNode is used to return the local Node
Node *memberlist::LocalNode()
{
    lock_guard<mutex> l(nodeMutex);
    return &nodeMap[config->Name]->N;
}

// Members returns a list of all known live nodes. The node structures
// returned must not be modified. If you wish to modify a Node, make a
// copy first.
vector<Node *> memberlist::Members()
{
    lock_guard<mutex> l(nodeMutex);
    vector<Node *> v;
    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (!nodes[i]->DeadOrLeft())
        {
            v.push_back(&nodes[i]->N);
        }
    }
    return v;
}

// NumMembers returns the number of alive nodes currently known. Between
// the time of calling this and calling Members, the number of alive nodes
// may have changed, so this shouldn't be used to determine how many
// members will be returned by Members.
size_t memberlist::NumMembers()
{
    lock_guard<mutex> l(nodeMutex);
    size_t alive = 0;
    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (!nodes[i]->DeadOrLeft())
        {
            alive++;
        }
    }
    return alive;
}

uint32_t memberlist::EstNumNodes()
{
    return numNodes.load();
}

// SendBestEffort uses the unreliable packet-oriented interface of the transport
// to target a user message at the given node (this does not use the gossip
// mechanism). The maximum size of the message depends on the configured
// UDPBufferSize for this memberlist instance.
void memberlist::SendBestEffort(shared_ptr<Node> to, string msg)
{
    auto user = genUser(msg);
    sockaddr_in a = to->FullAddr();
    encodeSendUDP(udpFd, &a, user);
}

// SendReliable uses the reliable stream-oriented interface of the transport to
// target a user message at the given node (this does not use the gossip
// mechanism). Delivery is guaranteed if no error is returned, and there is no
// limit on the size of the message.
void memberlist::SendReliable(shared_ptr<Node> to, string msg)
{
    auto user = genUser(msg);
    sockaddr_in a = to->FullAddr();
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    ConnectTimeout(fd, (struct sockaddr *)&a, sizeof(sockaddr), config->TCPTimeout);
    encodeSendTCP(fd, user);
}
