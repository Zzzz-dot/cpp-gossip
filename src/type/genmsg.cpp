// Generate the corresponding message in protobuf type
#include <type/msgtype.pb.h>
#include <type/genmsg.h>

#include <iostream>
using namespace std;

void addMessage(Compound &cd, MessageData md_)
{
    auto mds = cd.mutable_mds();
    MessageData *md = mds->Add();
    *md = md_;
}

MessageData genPing(uint32_t seqno, const string &node, const string &sourceaddr, uint32_t sourceport, const string &sourcenode)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pingMsg);
    Ping *p = md.mutable_ping();
    p->set_seqno(seqno);
    p->set_node(node);
    p->set_sourceaddr(sourceaddr);
    p->set_sourceport(sourceport);
    p->set_sourcenode(sourcenode);
    return md;
}

MessageData genIndirectPing(uint32_t seqno, const string &node, const string &targetaddr, uint32_t targetport, bool nack, const string &sourceaddr, uint32_t sourceport, const string &sourcenode)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_indirectPingMsg);
    IndirectPing *p = md.mutable_indirectping();
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

MessageData genAckResp(uint32_t seqno)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_ackRespMsg);
    AckResp *p = md.mutable_ackresp();
    p->set_seqno(seqno);
    return md;
}

MessageData genNackResp(uint32_t seqno)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_nackRespMsg);
    NackResp *p = md.mutable_nackresp();
    p->set_seqno(seqno);
    return md;
}

MessageData genErrResp(const string &error)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_errMsg);
    ErrResp *p = md.mutable_errresp();
    p->set_error(error);
    return md;
}

MessageData genUser(const string &msg){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_userMsg);
    User *p = md.mutable_user();
    p->set_msg(msg);
    return md;
}

MessageData genComMsg(ComBroadcast &cbc){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_compoundBroad);
    ComBroadcast *p=md.mutable_combroadcast();
    *p=cbc;
    return md;
}

MessageData genSuspectMsg(uint32_t incarnation,const string &node,const string &from){
    ComBroadcast cbc;
    Broadcast bc=genSuspectBroadcast(incarnation,node,from);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}
MessageData genSuspectMsg(const Suspect &s){
    ComBroadcast cbc;
    Broadcast bc=genSuspectBroadcast(s);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}

MessageData genDeadMsg(uint32_t incarnation,const string &node,const string &from){
    ComBroadcast cbc;
    Broadcast bc=genDeadBroadcast(incarnation,node,from);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}
MessageData genDeadMsg(const Dead &d){
    ComBroadcast cbc;
    Broadcast bc=genDeadBroadcast(d);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}

MessageData genAliveMsg(uint32_t incarnation,const string &node,const string &addr,uint32_t port){
    ComBroadcast cbc;
    Broadcast bc=genAliveBroadcast(incarnation,node,addr,port);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}
MessageData genAliveMsg(const Alive &a){
    ComBroadcast cbc;
    Broadcast bc=genAliveBroadcast(a);
    addBroadCast(cbc,bc);
    MessageData md=genComMsg(cbc);
    return md;
}

Broadcast genSuspectBroadcast(uint32_t incarnation, const string &node, const string &from)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_suspectMsg);
    Suspect *p = bc.mutable_suspect();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_from(from);
    return bc;
}
Broadcast genSuspectBroadcast(const Suspect &s)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_suspectMsg);
    Suspect *p = bc.mutable_suspect();
    *p=s;
    return bc;
}

Broadcast genAliveBroadcast(uint32_t incarnation, const string &node, const string &addr, uint32_t port)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_aliveMsg);
    Alive *p = bc.mutable_alive();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_addr(addr);
    p->set_port(port);
    return bc;
}
Broadcast genAliveBroadcast(const Alive &a)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_aliveMsg);
    Alive *p = bc.mutable_alive();
    *p=a;
    return bc;
}

Broadcast genDeadBroadcast(uint32_t incarnation, const string &node, const string &from)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_deadMsg);
    Dead *p = bc.mutable_dead();
    p->set_incarnation(incarnation);
    p->set_node(node);
    p->set_from(from);
    return bc;
}
Broadcast genDeadBroadcast(const Dead &d)
{
    Broadcast bc;
    bc.set_type(Broadcast_BroadcastType_deadMsg);
    Dead *p = bc.mutable_dead();
    *p=d;
    return bc;
}

MessageData genPushPull(bool join)
{
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pushPullMsg);
    PushPull *p = md.mutable_pushpull();
    p->set_join(join);
    return md;
}

void addPushNodeState(MessageData &md, const string &name, const string &addr, uint32_t port, uint32_t incarnation, PushNodeState::NodeStateType state)
{
    PushPull *p = md.mutable_pushpull();
    auto nds = p->mutable_states();
    PushNodeState *nd = nds->Add();
    nd->set_name(name);
    nd->set_addr(addr);
    nd->set_port(port);
    nd->set_incarnation(incarnation);
    nd->set_state(state);
}

Suspect getSuspect(uint32_t incarnation, const string &node, const string &from)
{
    Suspect s;
    s.set_incarnation(incarnation);
    s.set_node(node);
    s.set_from(from);
    return s;
}

Alive getAlive(uint32_t incarnation, const string &node, const string &addr, uint32_t port)
{
    Alive a;
    a.set_incarnation(incarnation);
    a.set_node(node);
    a.set_addr(addr);
    a.set_port(port);
    return a;
}

Dead getDead(uint32_t incarnation, const string &node, const string &from)
{
    Dead d;
    d.set_incarnation(incarnation);
    d.set_node(node);
    d.set_from(from);
    return d;
}

void addBroadCast(ComBroadcast &cbc, Broadcast bc_){
    auto bs=cbc.mutable_bs();
    Broadcast *bc=bs->Add();
    *bc=bc_;
}