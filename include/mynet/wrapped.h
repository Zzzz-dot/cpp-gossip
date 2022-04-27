#ifndef _WRAPPED_H
#define _WRAPPED_H

#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

int Socket(int domain, int type, int protocol);

void Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len);

void Listen(int fd, int n);

void Connect(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len);

int Accept(int fd, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len);

ssize_t Write(int fd, const void *buf, size_t n);

ssize_t Read(int fd, void *buf, size_t n);

void Close(int fd);

ssize_t Sendto(int fd, const void *buf, size_t n, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len);

ssize_t Recvfrom(int fd, void *__restrict buf, size_t n, int flags, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len);

int Epoll_create(int size);

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

void Pipe(int *pipedes);

void Pipe2(int *pipedes,int flags);

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval* timeout);
#endif
