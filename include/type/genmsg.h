// Generate the corresponding message in protobuf type
#ifndef _GENMSG_HPP
#define _GENMSG_HPP

#include "msgtype.pb.h"

#include <iostream>

void addMessage(Compound &cd, MessageData md_);

MessageData genPing(uint32_t seqno,const std::string &node,const std::string &sourceaddr,uint32_t sourceport,const std::string &sourcenode);

MessageData genIndirectPing(uint32_t seqno,const std::string &node,const std::string &targetaddr,uint32_t targetport,bool nack,const std::string &sourceaddr,uint32_t sourceport,const std::string &sourcenode);

MessageData genAckResp(uint32_t seqno);

MessageData genNackResp(uint32_t seqno);

MessageData genPushPull(bool join);
void addPushNodeState(MessageData &md,const std::string& name,const std::string& addr,uint32_t port,uint32_t incarnation,PushNodeState::NodeStateType state);

MessageData genErrResp(const std::string &error);

MessageData genUser(const std::string &msg);

MessageData genComMsg(ComBroadcast &cbc);

MessageData genSuspectMsg(uint32_t incarnation,const std::string &node,const std::string &from);
MessageData genSuspectMsg(const Suspect &s);

MessageData genDeadMsg(uint32_t incarnation,const std::string &node,const std::string &from);
MessageData genDeadMsg(const Dead &d);

MessageData genAliveMsg(uint32_t incarnation,const std::string &node,const std::string &addr,uint32_t port);
MessageData genAliveMsg(const Alive &a);

Suspect getSuspect(uint32_t incarnation,const std::string &node,const std::string &from);

Alive getAlive(uint32_t incarnation,const std::string &node,const std::string &addr,uint32_t port);

Dead getDead(uint32_t incarnation,const std::string &node,const std::string &from);

Broadcast genSuspectBroadcast(uint32_t incarnation,const std::string &node,const std::string &from);
Broadcast genSuspectBroadcast(const Suspect &s);

Broadcast genAliveBroadcast(uint32_t incarnation,const std::string &node,const std::string &addr,uint32_t port);
Broadcast genAliveBroadcast(const Alive &a);

Broadcast genDeadBroadcast(uint32_t incarnation,const std::string &node,const std::string &from);
Broadcast genDeadBroadcast(const Dead &d);

void addBroadCast(ComBroadcast &cbc, Broadcast bc_);


#endif