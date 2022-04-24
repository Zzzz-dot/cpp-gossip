#ifndef _NODE_H
#define _NODE_H

#include <misc/timer.hpp>

#include <iostream>
#include <arpa/inet.h>
#include <chrono>
#include <functional>
using namespace std;

enum NodeStateType
{
    StateAlive = 0,
    StateSuspect,
    StateDead,
    StateLeft
};

// Node represents a node in the cluster.
typedef struct Node
{
    string Name;
    string Addr;
    uint16_t Port;
    NodeStateType State;

    struct sockaddr_in FullAddr()
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
} Node;

// NodeState is used to manage our state view of another node
typedef struct NodeState
{
    Node Node;
    uint32_t Incarnation;                                        // Last known incarnation number
    NodeStateType State;                                         // Current state
    chrono::duration<int64_t, chrono::microseconds> StateChange; // Time last state change happened
    
    bool DeadOrLeft(){
        return State==StateDead||State==StateLeft;
    }
    
} NodeState;

// AckHandler contains callback function when AckResp
typedef struct AckHandler{
    function<void(int64_t)> ackFn;
    function<void()> nackFn;
    timer t;
    AckHandler(const function<void(int64_t)> &ackfn_,const function<void()> &nackfn_,const timer &t_):ackFn(ackfn_),nackFn(nackfn_),t(t_){};
}AckHandler;

typedef struct ackMessage{
    bool Complete;
    int64_t Timestamp;
    ackMessage()=default;
    ackMessage(uint32_t complete,int64_t timestamp):Complete(complete),Timestamp(timestamp){};
}ackMessage;

typedef struct Suspicion{

}Suspicion;

#endif