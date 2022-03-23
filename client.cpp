#include "net/wrapped.h"
#include "net/net.h"
#include <arpa/inet.h>
#include <string.h>

int main(int argc,char **argv){

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    if(int e=inet_pton(AF_INET,argv[1],&server_addr.sin_addr)<=0){
        errno=e;
    }

    const char *msg2="Hello UDP";
    SendUdpMsg(server_addr,msg2,strlen(msg2));

    const char *msg1="Hello TCP";
    SendTcpMsg(server_addr,msg1,strlen(msg1));


    return 0;

}