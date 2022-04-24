// Generate the corresponding message in protobuf type
#ifndef _GENMSG_HPP
#define _GENMSG_HPP

#include "msgtype.pb.h"

#include <iostream>

using namespace std;

MessageData genPing(uint32_t seqno,const string &node,const string &sourceaddr,uint32_t sourceport,const string &sourcenode);

MessageData genIndirectPing(uint32_t seqno,const string &node,const string &targetaddr,uint32_t targetport,bool nack,const string &sourceaddr,uint32_t sourceport,const string &sourcenode);

MessageData genAckResp(uint32_t seqno);

MessageData genNackResp(uint32_t seqno);

MessageData genErrResp(const string &error);

MessageData genSuspect(uint32_t incarnation,const string &node,const string &from);

MessageData genAlive(uint32_t incarnation,const string &node,const string &addr,uint32_t port);

MessageData genDead(uint32_t incarnation,const string &node,const string &from);

MessageData genPushPull(bool join);

void addPushNodeState(MessageData &md,const string& name,const string& addr,uint32_t port,uint32_t incarnation,PushNodeState::NodeStateType state);


#endif