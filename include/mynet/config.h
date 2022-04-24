#ifndef _CONFIG_H
#define _CONFIG_H

#include <iostream>

using namespace std;

//该 gossip 协议设计涉及两种传输层数据类型，分别为 TCP 和 UDP
//通过 UDP 发送 Probe 消息探测其他节点的状态
//通过 UDP 发送 Gossip 消息，传播节点的更新信息
//通过 TCP 发送 PushPull 消息，交换所有节点的信息
typedef struct config
{
    // The name of this node. This must be unique in the cluster.
    string Name;

    // Configuration related to what address to bind to and ports to
	// listen on. The port is used for both UDP and TCP gossip.
	string BindAddr;
	uint16_t BindPort;

	// TCPTimeout is the timeout for establishing a stream connection with
	// a remote node for a full state sync, and for stream read and write
	// operations.
	uint32_t TCPTimeout;

	// IndirectChecks is the number of nodes that will be asked to perform
	// an indirect probe of a node in the case a direct probe fails. Memberlist
	// waits for an ack from any single indirect node, so increasing this
	// number will increase the likelihood that an indirect probe will succeed
	// at the expense of bandwidth.
	uint8_t IndirectChecks;

	// RetransmitMult is the multiplier for the number of retransmissions
	// that are attempted for messages broadcasted over gossip. The actual
	// count of retransmissions is calculated using the formula:
	//
	//   Retransmits = RetransmitMult * log(N+1)
	//
	// This allows the retransmits to scale properly with cluster size. The
	// higher the multiplier, the more likely a failed broadcast is to converge
	// at the expense of increased bandwidth.
	uint8_t RetransmitMult;

	// SuspicionMult is the multiplier for determining the time an
	// inaccessible node is considered suspect before declaring it dead.
	// The actual timeout is calculated using the formula:
	//
	//   SuspicionTimeout = SuspicionMult * log(N+1) * ProbeInterval
	//
	// This allows the timeout to scale properly with expected propagation
	// delay with a larger cluster size. The higher the multiplier, the longer
	// an inaccessible node is considered part of the cluster before declaring
	// it dead, giving that suspect node more time to refute if it is indeed
	// still alive.
	uint8_t SuspicionMult;


	// SuspicionMaxTimeoutMult is the multiplier applied to the
	// SuspicionTimeout used as an upper bound on detection time. This max
	// timeout is calculated using the formula:
	//
	// SuspicionMaxTimeout = SuspicionMaxTimeoutMult * SuspicionTimeout
	//
	// If everything is working properly, confirmations from other nodes will
	// accelerate suspicion timers in a manner which will cause the timeout
	// to reach the base SuspicionTimeout before that elapses, so this value
	// will typically only come into play if a node is experiencing issues
	// communicating with other nodes. It should be set to a something fairly
	// large so that a node having problems will have a lot of chances to
	// recover before falsely declaring other nodes as failed, but short
	// enough for a legitimately isolated node to still make progress marking
	// nodes failed in a reasonable amount of time.
	uint8_t SuspicionMaxTimeoutMult; 


	// PushPullInterval is the interval between complete state syncs.
	// Complete state syncs are done with a single node over TCP and are
	// quite expensive relative to standard gossiped messages. Setting this
	// to zero will disable state push/pull syncs completely.
	//
	// Setting this interval lower (more frequent) will increase convergence
	// speeds across larger clusters at the expense of increased bandwidth
	// usage.
	uint32_t PushPullInterval;


	// ProbeInterval and ProbeTimeout are used to configure probing
	// behavior for memberlist.
	//
	// ProbeInterval is the interval between random node probes. Setting
	// this lower (more frequent) will cause the memberlist cluster to detect
	// failed nodes more quickly at the expense of increased bandwidth usage.
	//
	// ProbeTimeout is the timeout to wait for an ack from a probed node
	// before assuming it is unhealthy. This should be set to 99-percentile
	// of RTT (round-trip time) on your network.
	uint32_t ProbeInterval;
	uint32_t ProbeTimeout;

	// GossipInterval and GossipNodes are used to configure the gossip
	// behavior of memberlist.
	//
	// GossipInterval is the interval between sending messages that need
	// to be gossiped that haven't been able to piggyback on probing messages.
	// If this is set to zero, non-piggyback gossip is disabled. By lowering
	// this value (more frequent) gossip messages are propagated across
	// the cluster more quickly at the expense of increased bandwidth.
	//
	// GossipNodes is the number of random nodes to send gossip messages to
	// per GossipInterval. Increasing this number causes the gossip messages
	// to propagate across the cluster more quickly at the expense of
	// increased bandwidth.
	//
	// GossipToTheDeadTime is the interval after which a node has died that
	// we will still try to gossip to it. This gives it a chance to refute.
	uint32_t GossipInterval;
	uint8_t GossipNodes;
	uint32_t GossipToTheDeadTime;
};


#endif