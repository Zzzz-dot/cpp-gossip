// Generate the corresponding message in protobuf type
#include <type/msgtype.pb.h>
#include <type/genmsg.h>

#include <iostream>
using namespace std;

MessageData genPing(uint32_t seqno,const string &node,const string &sourceaddr,uint32_t sourceport,const string &sourcenode){
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

MessageData genIndirectPing(uint32_t seqno,const string &node,const string &targetaddr,uint32_t targetport,bool nack,const string &sourceaddr,uint32_t sourceport,const string &sourcenode){
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

MessageData genAckResp(uint32_t seqno){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_ackRespMsg);
    AckResp* p=md.mutable_ackresp();
    p->set_seqno(seqno);
    return md;
}

MessageData genNackResp(uint32_t seqno){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_nackRespMsg);
    NackResp* p=md.mutable_nackresp();
    p->set_seqno(seqno);
    return md;
}

MessageData genErrResp(const string &error){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_errMsg);
    ErrResp* p=md.mutable_errresp();
    p->set_error(error);
    return md;
}

MessageData genSuspect(uint32_t incarnation,const string &node,const string &from){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_suspectMsg);
    Suspect* p=md.mutable_suspect();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_from(from);
    return md;
}

MessageData genAlive(uint32_t incarnation,const string &node,const string &addr,uint32_t port){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_aliveMsg);
    Alive* p=md.mutable_alive();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_addr(addr);
    p->set_port(port);
    return md;
}

MessageData genDead(uint32_t incarnation,const string &node,const string &from){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_deadMsg);
    Dead* p=md.mutable_dead();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_from(from);
    return md;
}

MessageData genPushPull(bool join){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pushPullMsg);
    PushPull* p=md.mutable_pushpull();
    p->set_join(join);
    return md;
}

void addPushNodeState(MessageData &md,const string& name,const string& addr,uint32_t port,uint32_t incarnation,PushNodeState::NodeStateType state){
    PushPull* p=md.mutable_pushpull();
    auto nds=p->mutable_states();
    PushNodeState *nd=nds->Add();
    nd->set_name(name);
    nd->set_addr(addr);
    nd->set_port(port);
    nd->set_incarnation(incarnation);
    nd->set_state(state);
}