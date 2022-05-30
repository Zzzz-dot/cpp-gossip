#include <misc/node.h>

using namespace std;

Node::Node(const std::string &name, const std::string &addr, uint16_t port) : Name(name), Addr(addr), Port(port){};

Node::Node(const Node &node) : Name(node.Name), Addr(node.Addr), Port(node.Port){};

sockaddr_in Node::FullAddr()
{
    struct sockaddr_in fulladdr;
    fulladdr.sin_family = AF_INET;
    fulladdr.sin_port = htons(Port);
    if (inet_pton(AF_INET, Addr.c_str(), &fulladdr.sin_addr) <= 0)
    {
        exit(-1);
    }
    return fulladdr;
}

nodeState::nodeState(const Node &node, uint32_t incarnation, NodeStateType state) : N(node), Incarnation(incarnation), State(state){};

bool nodeState::DeadOrLeft()
{
    return State == StateDead || State == StateLeft;
}

ackHandler::ackHandler(const function<void(int64_t)> &ackfn_, const function<void()> &nackfn_, const onceTimer &t_) : ackFn(ackfn_), nackFn(nackfn_), t(t_){};

ackMessage::ackMessage(uint32_t complete, int64_t timestamp) : Complete(complete), Timestamp(timestamp){};