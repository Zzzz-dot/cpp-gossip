#include "net/wrapped.h"
#include "net/net.h"
#include "memberlist/type/msgtype.pb.h"
#include <arpa/inet.h>
#include <string.h>

int main(int argc,char **argv){

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    if(int e=inet_pton(AF_INET,argv[1],&server_addr.sin_addr)<=0){
        errno=e;
    }

    MessageData md;
    md.set_head(MessageData_MessageType::MessageData_MessageType_pingMsg);
    Ping* p=md.mutable_ping();
    p->set_seqno(1);
    p->set_node("as");
    p->set_sourceaddr("asa");
    p->set_sourcenode("af");
    p->set_sourceport(123);
    cout<<md.DebugString()<<endl;

    int fd=Socket(AF_INET,SOCK_DGRAM,0);
    encodeSendUDP(fd,&server_addr,md);


    encodeSendTCP(&server_addr,md);


    return 0;

}