#include <mynet/wrapped.h>
#include <type/msgtype.pb.h>

#include <string.h>
#include <thread>
#include <arpa/inet.h>
#include <iostream>

int main()
{

    //IPv4 Internet protocols, TCP, Default Protocol
    int server_fd_tcp = Socket(AF_INET, SOCK_STREAM, 0);
    //IPv4 Internet protocols, UDP, Default Protocol
    int server_fd_udp = Socket(AF_INET, SOCK_DGRAM, 0);

    //socketaddr_in and socketaddr have same size, we use socketaddr_in for system compatibility reasons
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(8888);
    //Accept connection from every ip address (This is used for multi-host)
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(server_fd_tcp, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    Bind(server_fd_udp, (struct sockaddr *)&local_addr, sizeof(sockaddr));

    //Start Listen TCP
    Listen(server_fd_tcp, 1);

    //TCP Server
    auto tcp_server = [server_fd_tcp]() {
        int client_fd;
        struct sockaddr_in remote_addr;
        socklen_t sin_size;
        printf("Start TCP\n");
        for (;;)
        {
            client_fd = Accept(server_fd_tcp, (struct sockaddr *)&remote_addr, &sin_size);
            char addr[INET6_ADDRSTRLEN];
            in_port_t port=ntohs(remote_addr.sin_port);
            inet_ntop(AF_INET,&remote_addr.sin_addr,addr,sizeof(addr));
            printf("Accept TCP Link from %s:%hhu\n",addr,port);
            char buf[1024];
            int n;
            while ((n=Read(client_fd,buf,1024))>0){
                buf[n]=0;
                MessageData md;
                md.ParseFromString(buf);
                std::cout<<md.DebugString()<<std::endl;
            }
            Close(client_fd);
        }
    };

    //UDP Server
    auto udp_server = [server_fd_udp]() {
        int client_fd;
        struct sockaddr_in remote_addr;
        socklen_t sin_size;
        printf("Start UDP\n");
        for (;;)
        {
            char buf[1024];
            int n;
            while ((n=Recvfrom(server_fd_udp,buf,1024,0,(struct sockaddr *)&remote_addr, &sin_size))>0){
                buf[n]=0;
                MessageData md;
                md.ParseFromString(buf);
                std::cout<<md.DebugString()<<std::endl;
            }
        }
    };

    std::thread t1(tcp_server);
    sleep(5);
    std::thread t2(udp_server);
    t1.join();
    t2.join();

    return 0;
}
