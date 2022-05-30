#ifndef _NODE_H
#define _NODE_H

#include "timer.h"

#include <iostream>
#include <functional>
#include <arpa/inet.h>

enum NodeStateType
{
    StateAlive = 0,
    StateSuspect,
    StateDead,
    StateLeft
};

// Node represents a node in the cluster.
struct Node
{
    std::string Name;
    std::string Addr;
    uint16_t Port;

    Node() = default;
    Node(const std::string &name, const std::string &addr, uint16_t port);
    Node(const Node &node);

    sockaddr_in FullAddr();
};

// nodeState is used to manage our state view of another node
struct nodeState
{
    Node N;
    uint32_t Incarnation; // Last known incarnation number
    NodeStateType State;  // Current state
    int64_t StateChange;  // Time last state change happened
    // chrono::duration<int64_t, chrono::microseconds> StateChange;

    nodeState() = default;
    nodeState(const Node &node, uint32_t incarnation, NodeStateType state);


    bool DeadOrLeft();

};

// ackHandler contains callback function when AckResp
struct ackHandler
{
    std::function<void(int64_t)> ackFn;
    std::function<void()> nackFn;
    onceTimer t;
    ackHandler(const std::function<void(int64_t)> &ackfn_, const std::function<void()> &nackfn_, const onceTimer &t_);
};

struct ackMessage
{
    bool Complete;
    int64_t Timestamp;
    ackMessage() = default;
    ackMessage(uint32_t complete, int64_t timestamp);
};

#endif