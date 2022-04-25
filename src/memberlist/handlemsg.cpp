#include <memberlist/memberlist.h>

void memberlist::handleconn(int sockfd){
    struct sockaddr_in remote_addr;
    bzero(&remote_addr, sizeof(sockaddr_in));
    socklen_t socklen = sizeof(sockaddr_in);

    int connfd = Accept(sockfd, (struct sockaddr *)&remote_addr, &socklen);

    struct epoll_event ev;
    //EPOLLET
    ev.events = EPOLLIN;
    ev.data.fd = connfd;
    Epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);

#ifdef PRINT_ADDRINFO
    printaddr(remote_addr);
#endif
}

void memberlist::handletcp(int sockfd){

}

void memberlist::handleudp(int sockfd){
    struct sockaddr_in remote_addr;
    bzero(&remote_addr, sizeof(sockaddr_in));
    socklen_t socklen = sizeof(sockaddr_in);  

    auto md=onReceiveUDP(sockfd,remote_addr,socklen);
    switch (md.head())
    {
    case MessageData::MessageType::MessageData_MessageType_pingMsg:
        handlePing(md,remote_addr);
        break;
    default:
        break;
    }
}

void memberlist::handlePing(MessageData &ping,sockaddr_in &remote_addr){
    string addr;
    if (ping.ping().sourceaddr()!=""){
        addr=ping.ping().sourceaddr();
    }else{
        char addr_[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&remote_addr.sin_addr,addr_,sizeof(addr_));
        addr=string(addr_);
    }
    if (ping.ping().node()!=""&&ping.ping().node()!=config.Name){
        logger<<"[WARN] memberlist: Got ping for unexpected node \'"<<ping.ping().node()<<"\' from "<<addr<<endl;
    }

    auto ackmsg=genAckResp(ping.ping().seqno());

    encodeSendUDP(udpfd,&remote_addr,ackmsg);
}



void memberlist::handleevent()
{
    auto handleevent = [this]()
    {
        for (;;)
        {
            int events_num = Epoll_wait(this->epollfd, this->events, EPOLLSIZE, -1);
            for (int i = 0; i < events_num; i++)
            {
                int socketfd = this->events[i].data.fd;

                // Receive a new connection from TCP
                if (socketfd == this->tcpfd)
                {
                    handleconn(socketfd);
                }
                // Receive a new message from UDP
                else if (socketfd == this->udpfd)
                {
                    handletcp(socketfd);
                }
                // Receive a new message from TCP
                else
                {
                    handleudp(socketfd);
                }
            }
        }
    };
    //Add a thread pool to handle different kind of message;
    std::thread(handleevent);
}