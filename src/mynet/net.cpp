#include <mynet/net.h>

MessageData decodeReceiveUDP(int fd, sockaddr_in &remote_addr, socklen_t &sin_size)
{
    MessageData md;
    char buf[1024];
    int n = Recvfrom(fd, buf, 1024, 0, (struct sockaddr *)&remote_addr, &sin_size);
    buf[n] = 0;
    if (md.ParseFromString(buf) == false)
    {
        cout << "ParseFromString Error!" << endl;
    }
#ifdef DEBUG
    cout << "[DEBUG] Receive UDP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    return md;
}

MessageData decodeReceiveTCP(int fd)
{
    MessageData md;
    char buf[1024];
    int n = Read(fd, buf, 1024);
    buf[n] = 0;
    if (md.ParseFromString(buf) == false)
    {
        cout << "ParseFromString Error!" << endl;
    }
#ifdef DEBUG
    cout << "[DEBUG] Receive TCP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    return md;
}

void beforeSend(const MessageData &md, string *s)
{
    if (md.SerializeToString(s) == false)
    {
        cout << "SerializeToString Error!" << endl;
        return;
    }
}

string beforeSend(const MessageData &md)
{
    string s = md.SerializeAsString();
    if (s.empty())
    {
        cout << "SerializeAsString Error!" << endl;
    }
    return s;
}

void encodeSendTCP(int fd, const MessageData &md)
{
#ifdef DEBUG
    cout << "[DEBUG] Send TCP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    string encodeMsg = beforeSend(md);
    Write(fd, encodeMsg.c_str(), encodeMsg.size());
};

void encodeSendUDP(int fd, const struct sockaddr_in *remote_addr, const MessageData &md)
{
#ifdef DEBUG
    cout << "[DEBUG] Send UDP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    string encodeMsg = beforeSend(md);
    Sendto(fd, encodeMsg.c_str(), encodeMsg.size(), 0, (struct sockaddr *)remote_addr, sizeof(sockaddr));
};



// Socket timeout setting mode:

// 1. Call alarm, which generates SIGALARM when the specified timeout expires. This approach involves signal processing,
//    which varies from implementation to implementation and can interfere with existing alarm calls in the process.

// 2. Blocking waits for I/O in SELECT (select has a built-in time limit) instead of directly blocking on read or write calls.

// 3. Use the SO_RECVTIMEO and SO_SNDTIMEO socket options

int connectTimeout(int fd, const struct sockaddr *addr, socklen_t len, uint32_t timeout)
{
    int flags, n, error;

    // Non-blocking
    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    n = connect(fd, addr, len);
    if (n < 0 && errno != EINPROGRESS)
    {
        return -1;
    }

    if (n == 0)
    {
        goto done;
    }

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    wset = rset;

    struct timeval connecttimeout;
    connecttimeout.tv_usec = timeout;

    n = Select(fd + 1, &rset, &wset, nullptr, &connecttimeout);
    if (n == 0)
    {
        close(fd);
        errno = ETIMEDOUT;
        return -1;
    }

    error = 0;
    if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset))
    {
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, NULL) < 0)
            return -1;
    }

done:
    fcntl(fd, F_SETFL, flags);
    if (error)
    {
        close(fd);
        errno = error;
        return -1;
    }
    return 0;
}

void ConnectTimeout(int fd, const struct sockaddr *addr, socklen_t len, uint32_t timeout)
{
    int e = connectTimeout(fd, addr, len, timeout);
    if (e < 0)
    {
        // do something
    }
}

// sendAndReceiveState is used to initiate a push/pull over a stream with a
// remote host.
MessageData memberlist::sendAndReceiveState(struct sockaddr_in &remote_node, bool join)
{
    // This version of the implementation does not contain TCPTimeout

    // Connection Timeout
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    ConnectTimeout(fd, (struct sockaddr *)&remote_node, sizeof(sockaddr), config.TCPTimeout);

    struct timeval tcptimeout;
    tcptimeout.tv_usec = config.TCPTimeout;

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tcptimeout, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tcptimeout, sizeof(struct timeval));

    sendLocalState(fd, join);

    auto pushpull=decodeReceiveTCP(fd);

    if(pushpull.head()==MessageData::MessageType::MessageData_MessageType_errMsg){
        logger<<"Remote error: "<<pushpull.errresp().error()<<endl;
        return;
    }

    if(pushpull.head()!=MessageData::MessageType::MessageData_MessageType_pushPullMsg){
        logger<<"Receive invalid msgType: "<<pushpull.head()<<", expected: "<<MessageData::MessageType::MessageData_MessageType_pushPullMsg<<endl;
        return;
    }
    return pushpull;
}

// sendLocalState is invoked to send our local state over a stream connection.
void memberlist::sendLocalState(int fd, bool join)
{
    // This version of the implementation does not contain TCPTimeout
    auto pushpull = genPushPull(join);
    {
        lock_guard<mutex> l(nodeMutex);
        for (size_t i = 0; i < nodes.size(); i++)
        {
            addPushNodeState(pushpull, nodes[i]->Node.Name, nodes[i]->Node.Addr, nodes[i]->Node.Port, nodes[i]->Incarnation, PushNodeState::NodeStateType(nodes[i]->State));
        }
    }
    encodeSendTCP(fd, pushpull);
}

void memberlist::mergeRemoteState(MessageData &pushpull){
    auto &states=pushpull.pushpull().states();
    int size=pushpull.pushpull().states_size();
    for (int i=0;i<size;i++){
        auto &state=states[i];
        switch (state.state())
        {
        case PushNodeState::StateAlive:
            auto a=getAlive(state.incarnation(),state.name(),state.addr(),state.port());
            aliveNode(a,false,NULL);
            break;
        case PushNodeState::StateLeft:

            break;
        case PushNodeState::StateDead:
			// If the remote node believes a node is dead, we prefer to
			// suspect that node instead of declaring it dead instantly
        case PushNodeState::StateSuspect:

            break;
        }
    }
}

void memberlist::mergeRemoteState(vector<NodeState>& v){
    
}