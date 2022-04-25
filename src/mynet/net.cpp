#include <mynet/net.h>

/* #define SWITCH(cond)                                                       \
    switch (md.head())                                                     \
    {                                                                      \
    case MessageData_MessageType::MessageData_MessageType_pingMsg:         \
                                                                           \
        break;                                                             \
    case MessageData_MessageType::MessageData_MessageType_indirectPingMsg: \
                                                                           \
        break;                                                             \
    case MessageData_MessageType::MessageData_MessageType_ackRespMsg:      \
                                                                           \
        break;                                                             \
    case MessageData_MessageType::MessageData_MessageType_suspectMsg:      \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_aliveMsg:        \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_deadMsg:         \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_pushPullMsg:     \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_userMsg:         \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_nackRespMsg:     \
                                                                           \
    case MessageData_MessageType::MessageData_MessageType_errMsg:          \
                                                                           \
    default:                                                               \

    } */

MessageData onReceiveUDP(int fd, sockaddr_in &remote_addr, socklen_t &sin_size)
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
    cout<<"[DEBUG] Receive UDP Message:"<<endl;
    cout<<md.DebugString()<<endl;
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

void sendTCP(const struct sockaddr_in *remote_addr, const void *msg, size_t n)
{
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(fd, (struct sockaddr *)remote_addr, sizeof(sockaddr));
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

void sendUDP(int fd, const struct sockaddr_in *remote_addr, const void *msg, size_t n)
{
    Sendto(fd, msg, n, 0, (struct sockaddr *)remote_addr, sizeof(sockaddr));
}

void encodeSendTCP(const struct sockaddr_in *remote_addr, const MessageData &md)
{
#ifdef DEBUG
    cout << "[DEBUG] Send TCP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    string encodeMsg = beforeSend(md);
    sendTCP(remote_addr, encodeMsg.c_str(), encodeMsg.size());
};

void encodeSendUDP(int fd, const struct sockaddr_in *remote_addr, const MessageData &md)
{
#ifdef DEBUG
    cout << "[DEBUG] Send UDP Message:" << endl;
    cout << md.DebugString() << endl;
#endif
    string encodeMsg = beforeSend(md);
    sendUDP(fd, remote_addr, encodeMsg.c_str(), encodeMsg.size());
};

#endif