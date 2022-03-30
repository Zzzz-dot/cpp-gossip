#ifndef _MEMBERLIST_H
#define _MEMBERLIST_H
#include <net/config.h>
#include <net/wrapped.h>
#include <arpa/inet.h>

#include <string.h>

//TCP listen 最大监听的连接数
#define LISTENNUM 256

//epoll 最大监听的事件
#define EPOLLSIZE 1024

//是否打印出收到连接对端的地址信息
#define PRINT_ADDRINFO


class memberlist
{
private:
    uint32_t sequenceNum; // Local sequence number
    uint32_t incarnation; // Local incarnation number
    uint32_t numNodes;    // Number of known nodes (estimate)
    uint32_t pushPullReq; // Number of push/pull requests

    config config;

    int tcpfd;   // FD of the TCP Socket
    int udpfd;   // FD of the UDP Socker
    int epollfd; // FD of the epoll I/O MUX
    struct epoll_event events[EPOLLSIZE];   //Struct for holding epoll_event when epoll_wait returns

    

public:
    memberlist(/* args */);
    ~memberlist();
    void handlemsg();
    void schedule();

    //some helper functino
    #ifdef PRINT_ADDRINFO
    void printaddr(struct sockaddr_in &remote_addr){
        char addr[INET_ADDRSTRLEN];
        uint16_t port;
        if(inet_ntop(remote_addr.sin_family,&remote_addr.sin_addr,addr,sizeof(addr))<=0){
            exit(-1);
        }
        printf("Receive from %s:%u\n",addr,port);
    }
    #endif
};

void memberlist::handlemsg()
{
    auto handletcp = [](int coonfd) {
        Epoll_ctl(epollfd,)
        for(;;){

        }
    };

    auto handleevent=[this](){
        for(;;){
            int events_num=Epoll_wait(this->epollfd,this->events,EPOLLSIZE,-1);
            for (int i=0;i<events_num;i++){
                int socketfd=this->events[i].data.fd;
                struct sockaddr_in remote_addr;
                bzero(&remote_addr,sizeof(sockaddr_in));
                socklen_t socklen=sizeof(sockaddr_in);

                //Receive a new connection from TCP
                if(socketfd==this->tcpfd){
                    int connfd=Accept(socketfd,(struct sockaddr *)&remote_addr,&socklen);
                    handlemsg(connfd);
                }
                //Receive a new message from UDP
                else if(socketfd==this->udpfd){

                }
                //Receive a new message from TCP
                else{

                }
                #ifdef PRINT_ADDRINFO
                printaddr(remote_addr);
                #endif
            }
        }
    }
}

memberlist::memberlist(/* args */)
{
    //Init local address
    struct sockaddr_in local_addr;
    bzero(&local_addr,sizeof(sockaddr_in));
    local_addr.sin_family=AF_INET;
    local_addr.sin_port=htons(config.BindPort);
    if(int e=inet_pton(AF_INET,config.BindAddr.c_str(),&local_addr.sin_addr)<=0){
        errno=e;
    }

    //Create TCP socket
    tcpfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(tcpfd,(struct sockaddr*)&local_addr,sizeof(sockaddr));
    Listen(tcpfd,LISTENNUM);
    //Create UDP socket
    udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(udpfd,(struct sockaddr*)&local_addr,sizeof(sockaddr));
    //Create epoll
    epollfd = Epoll_create(EPOLLSIZE);

    //Add the tcpfd and udpfd to the watchlist of epollfd
    struct epoll_event ev1;
    ev1.events=EPOLLIN;
    ev1.data.fd=tcpfd;
    Epoll_ctl(epollfd,EPOLL_CTL_ADD,tcpfd,&ev1);
    struct epoll_event ev2;
    ev2.events=EPOLLIN;
    ev2.data.fd=udpfd;
    Epoll_ctl(epollfd,EPOLL_CTL_ADD,tcpfd,&ev2);
}

memberlist::~memberlist()
{
}

#endif