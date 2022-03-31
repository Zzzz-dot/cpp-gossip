#ifndef _MSGTYPE_H
#define _MSGTYPE_H
#include <cstdint>
#include <iostream>
#include <vector>
using namespace std;
// 消息头部附带消息种类
enum MessageType
{
    pingMsg = 0,
    indirectPingMsg,
    ackRespMsg,
    suspectMsg,
    aliveMsg,
    deadMsg,
    pushPullMsg,
    userMsg,
    nackRespMsg,
    errMsg
};

typedef struct MsgHead{
    MessageType Head;
    MsgHead(MessageType head):Head(head){};
}MsgHead;

// ping request sent directly to node
typedef struct Ping:public MsgHead
{
    uint32_t SeqNo;

    // Node is sent so the target can verify they are
    // the intended recipient. This is to protect again an agent
    // restart with a new name.
    string Node;

    string SourceAddr;   // Source address, used for a direct reply
    uint16_t SourcePort; // Source port, used for a direct reply
    string SourceNode;   // Source name, used for a direct reply

    Ping(uint32_t seqno,const string& node,const string& sourceaddr,uint16_t sourceport,const string& sourcenode):MsgHead(pingMsg){
        SeqNo=seqno;
        Node=node;
        SourceAddr=sourceaddr;
        SourcePort=sourceport;
        SourceNode=sourcenode;
    };

}Ping;

// indirect ping sent to an indirect node
// randomly choose k nodes and send indirect ping, these k nodes send ping to Node@TargetAddr:TargetPort
typedef struct IndirectPing:public MsgHead
{
    uint32_t SeqNo;   

    // Node is sent so the target can verify they are
    // the intended recipient. This is to protect again an agent
    // restart with a new name.
    string Node;
    string TargetAddr;
	uint16_t TargetPort;

    bool Nack;  // true if we'd like a nack back

    string SourceAddr;   // Source address, used for a direct reply
    uint16_t SourcePort; // Source port, used for a direct reply
    string SourceNode;   // Source name, used for a direct reply

    IndirectPing(uint32_t seqno,const string& node,const string& targetaddr,uint16_t targetport,bool nack,const string& sourceaddr,uint16_t sourceport,const string& sourcenode):MsgHead(indirectPingMsg){
        SeqNo=seqno;
        Node=node;
        TargetAddr=targetaddr;
        TargetPort=targetport;
        Nack=nack;
        SourceAddr=sourceaddr;
        SourcePort=sourceport;
        SourceNode=sourcenode;
    }

}IndirectPing;

// ack response is sent for a ping
typedef struct AckResp:public MsgHead{
	uint32_t SeqNo;   
	//vector<uint8_t> Payload;
    AckResp(uint32_t seqno):MsgHead(ackRespMsg){
        SeqNo=seqno;
    }
}AckResp;

// nack response is sent for an indirect ping when the pinger doesn't hear from
// the ping-ee within the configured timeout. This lets the original node know
// that the indirect ping attempt happened but didn't succeed.
typedef struct NackResp:public MsgHead{
	uint32_t SeqNo;
    NackResp(uint32_t seqno):MsgHead(nackRespMsg){
        SeqNo=seqno;
    } 
}NackResp;

// err response is sent to relay the error from the remote end
typedef struct ErrResp:public MsgHead{
	string Error; 
    ErrResp(const string& error):MsgHead(nackRespMsg){
        Error=error;
    } 
}ErrResp;

#endif