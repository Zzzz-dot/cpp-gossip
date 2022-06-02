#include <misc/util.h>

using namespace std;

bool hasPort(const string &s)
{
    size_t index = s.find(":");
    if (index == -1)
    {
        return false;
    }
    return true;
}

void ensurePort(string &s, uint16_t port)
{
    if (hasPort(s))
    {
        return;
    }
    else
    {
        s += ":" + to_string(port);
    }
}

struct sockaddr_in resolveAddr(const string &s)
{
    // cluster_add format: [name/]<ip>[:port]
    int index = s.find("/");
    string nodeName = "";
    string host;
    // if cluster_addr contains nodename
    // but nodename is not used in this implemention
    if (index != -1)
    {
        nodeName = s.substr(0, index);
        host = s.substr(index);
    }
    else
    {
        host = s;
    }

    ensurePort(host, DEFAULT_PORT);
    index = host.find(":");

    sockaddr_in cluster_addr;
    bzero((void *)&cluster_addr, sizeof(sockaddr_in));
    cluster_addr.sin_family = AF_INET;
    cluster_addr.sin_port = htons(stoul(host.substr(index + 1)));
    if (int e = inet_pton(AF_INET, host.substr(0, index).c_str(), &cluster_addr.sin_addr) <= 0)
    {
        errno = e;
    }
    return cluster_addr;
}

uint32_t retransmitLimit(uint8_t retransmitMult, uint32_t n)
{
    uint32_t nodeScale = ceil(log10(n + 1));
    uint32_t limit = retransmitMult * nodeScale;
    return limit;
}

vector<nodeState> kRandomNodes(uint8_t k, vector<shared_ptr<nodeState>> &nodes, function<bool(shared_ptr<nodeState>)> exclude)
{
    size_t n = nodes.size();
    vector<nodeState> kNodes;
    for (int i = 0; i < 3 * n && kNodes.size() < k; i++)
    {
        int idx = rand()%n;
        if (exclude != nullptr && exclude(nodes[idx]))
        {
            continue;
        }

        bool same = false;
        for (int j = 0; j < kNodes.size(); j++)
        {
            if (nodes[idx]->N.Name == kNodes[j].N.Name)
            {
                same = true;
            }
        }
        if (same == false)
        {
            kNodes.push_back(*nodes[idx]);
        }
    }
    return kNodes;
}

int64_t suspicionTimeout(uint32_t suspicionMult, uint32_t n, int64_t interval)
{
    double nodeScale = max(1.0, log10(1 + double(n)));
    int64_t timeout = suspicionMult * nodeScale * interval;
    return timeout;
}

string LogCoon(int connfd)
{
    struct sockaddr_in remote_addr;
    bzero(&remote_addr, sizeof(sockaddr_in));
    socklen_t socklen = sizeof(sockaddr_in);

    getpeername(connfd, (struct sockaddr *)&remote_addr, &socklen);

    return LogAddr(remote_addr);
}

string LogAddr(const sockaddr_in &remote_addr)
{
    char addr_[INET_ADDRSTRLEN];
    uint16_t port = remote_addr.sin_port;
    if (inet_ntop(remote_addr.sin_family, &remote_addr.sin_addr, addr_, sizeof(addr_)) <= 0)
    {
        //
    }
    string addr(addr_);
    addr += ":";
    addr += to_string(port);
    return addr;
}