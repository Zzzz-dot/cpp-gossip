#include <misc/config.h>
using namespace std;

shared_ptr<Config> DefaultLANConfig()
{
    char hostname_[256];
    gethostname(hostname_, sizeof(hostname_));
    string hostname(hostname_);

    auto c = make_shared<Config>();

    c->Name = hostname;
    c->BindAddr = "0.0.0.0";
    c->BindPort = DEFAULT_PORT;
    c->AdvertiseAddr = "";
    c->AdvertisePort = DEFAULT_PORT;
    c->TCPTimeout = chrono::microseconds(10s).count();
    c->IndirectChecks = 3;
    c->RetransmitMult = 4;
    c->SuspicionMult = 4;
    c->SuspicionMaxTimeoutMult = 6;
    c->PushPullInterval = chrono::microseconds(10s).count();
    c->ProbeInterval = chrono::microseconds(1s).count();
    c->ProbeTimeout = chrono::microseconds(500ms).count();
    c->AllowTcpPing = true;
    c->GossipInterval = chrono::microseconds(200ms).count();
    c->GossipNodes = 3;
    c->GossipToTheDeadTime = chrono::microseconds(30s).count();
    c->HandoffQueueDepth = 1024;
    c->UDPBufferSize = 1400;

    return c;
}

shared_ptr<Config> DefaultWANConfig()
{
    auto c = DefaultLANConfig();
    c->TCPTimeout = chrono::microseconds(30s).count();
    c->SuspicionMult = 6;
    c->PushPullInterval = chrono::microseconds(60s).count();
    c->ProbeInterval = chrono::microseconds(5s).count();
    c->ProbeTimeout = chrono::microseconds(3s).count();
    c->GossipInterval = chrono::microseconds(500ms).count();
    c->GossipNodes = 4;
    c->GossipToTheDeadTime = chrono::microseconds(60s).count();
    return c;
}

shared_ptr<Config> DefaultLocalConfig()
{
    auto c = DefaultLANConfig();
    c->AdvertiseAddr="127.0.0.1";
    c->TCPTimeout = chrono::microseconds(1s).count();
    c->IndirectChecks = 1;
    c->RetransmitMult = 2;
    c->SuspicionMult = 3;
    c->PushPullInterval = chrono::microseconds(15s).count();
    c->ProbeInterval = chrono::microseconds(1s).count();
    c->ProbeTimeout = chrono::microseconds(200ms).count();
    c->GossipInterval = chrono::microseconds(100ms).count();
    c->GossipToTheDeadTime = chrono::microseconds(15s).count();
    return c;
}

shared_ptr<Config> DebugConfig(){
    auto c=DefaultLocalConfig();
    c->PushPullInterval=chrono::microseconds(20s).count();
    c->ProbeInterval=chrono::microseconds(8s).count();
    c->GossipInterval=chrono::microseconds(4s).count();
    return c;
}