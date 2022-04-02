// Generate the corresponding message in protobuf type
#ifndef _GENMSG_HPP
#define _GENMSG_HPP
#include "msgtype.pb.h"
#include <iostream>
using namespace std;

MessageData& genPing(uint32_t seqno,const string &node,const string &sourceaddr,uint32_t sourceport,const string &sourcenode){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pingMsg);
    Ping* p=md.mutable_ping();
    p->set_seqno(seqno);
    p->set_node(node);
    p->set_sourceaddr(sourceaddr);
    p->set_sourceport(sourceport);
    p->set_sourcenode(sourcenode);
    return md;
}

MessageData& genIndirectPing(uint32_t seqno,const string &node,const string &targetaddr,uint32_t targetport,bool nack,const string &sourceaddr,uint32_t sourceport,const string &sourcenode){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_indirectPingMsg);
    IndirectPing* p=md.mutable_indirectping();
    p->set_seqno(seqno);
    p->set_node(node);
    p->set_targetaddr(targetaddr);
    p->set_targetport(targetport);
    p->set_nack(nack);
    p->set_sourceaddr(sourceaddr);
    p->set_sourceport(sourceport);
    p->set_sourcenode(sourcenode);
    return md;
}

MessageData& genAckResp(uint32_t seqno){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_ackRespMsg);
    AckResp* p=md.mutable_ackresp();
    p->set_seqno(seqno);
}

MessageData& genNackResp(uint32_t seqno){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_nackRespMsg);
    NackResp* p=md.mutable_nackresp();
    p->set_seqno(seqno);
}

MessageData& genErrResp(const string &error){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_errMsg);
    ErrResp* p=md.mutable_errresp();
    p->set_error(error);
}


#endif