#include <mynet/net.h>

using namespace std;

MessageData decodeReceiveUDP(int fd, sockaddr_in &remote_addr, socklen_t &sin_size)
{
    MessageData md;
    char buf[1024];
    int n = Recvfrom(fd, buf, 1024, 0, (sockaddr *)&remote_addr, &sin_size);
    buf[n] = 0;
    if (md.ParseFromString(buf) == false)
    {
        cout << "ParseFromString Error!" << endl;
    }
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
    string encodeMsg = beforeSend(md);
    Write(fd, encodeMsg.c_str(), encodeMsg.size());
};

void encodeSendUDP(int fd, const sockaddr_in *remote_addr, const MessageData &md)
{
    string encodeMsg = beforeSend(md);
    Sendto(fd, encodeMsg.c_str(), encodeMsg.size(), 0, (sockaddr *)remote_addr, sizeof(sockaddr));
};