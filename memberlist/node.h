#ifndef _NODE_H
#define _NODE_H
#include <iostream>
#include <arpa/inet.h>
#include <chrono>
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



typedef struct Suspicion{

}Suspicion;

#endif