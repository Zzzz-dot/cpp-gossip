#include <iostream>
using namespace std;

int main(){
    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pingMsg);
    Ping* p=md.mutable_ping();
    p->set_seqno(1);
    p->set_node("as");
    p->set_sourceaddr("asa");
    p->set_sourcenode("af");
    p->set_sourceport(123);
    cout<<md.DebugString()<<endl;
    cout<<md.head()<<endl;
    std::string s=SerializeAsString(md);
    cout<<s<<endl;
    MessageData md_;
    ParseFromString(md_,s);
    cout<<md_.DebugString()<<endl;
    return 0;

}