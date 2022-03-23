#ifndef _WRAPPED_H
#define _WRAPPED_H

#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>

#include <unistd.h>

int Socket(int domain, int type, int protocol){
    int n=socket(domain,type,protocol);
    if(n<0){
        errno=n;
    }
    return n;
}

void Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len){
    int e=bind(fd,addr,len);
    if(e<0){
        errno=e;
    }
}

void Listen(int fd, int n){
    int e=listen(fd,n);
    if(e<0){
        errno=e;
    }
}

int Accept(int fd, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len){
    int n=accept(fd,addr,addr_len);
    if(n<0){
        errno=n;
    }
    return n;
}

ssize_t Write(int fd, const void *buf, size_t n){
    ssize_t num=write(fd,buf,n);
    if(num<0){
        errno=num;
    }
    return num;
}

ssize_t Read(int fd, void *buf, size_t n){
    ssize_t num=read(fd,buf,n);
    if(num<0){
        errno=num;
    }
    return num;
}

void Connect(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len){
    int e=connect(fd,addr,len);
    if(e<0){
        errno=e;
    }
}

void Close(int fd){
    int e=close(fd);
    if(e<0){
        errno=e;
    }
}

ssize_t Sendto(int fd, const void *buf, size_t n, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len){
    ssize_t num=sendto(fd,buf,n,flags,addr,addr_len);
    if(num<0){
        errno=num;
    }
    return num;
}

ssize_t Recvfrom(int fd, void *__restrict buf, size_t n, int flags, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len){
    ssize_t num=recvfrom(fd,buf,n,flags,addr,addr_len);
    if(num<0){
        errno=num;
    }
    return num;
}

#endif