#include <mynet/wrapped.h>
using namespace std;

int Socket(int domain, int type, int protocol)
{
    int n = socket(domain, type, protocol);
    if (n < 0)
    {
        LOG(ERROR) << "socket error: " << strerror(errno) << endl;
    }
    return n;
}

void Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
    int e = bind(fd, addr, len);
    if (e < 0)
    {
        LOG(ERROR) << "bind error: " << strerror(errno) << endl;
    }
}

void Listen(int fd, int n)
{
    int e = listen(fd, n);
    if (e < 0)
    {
        LOG(ERROR) << "listen error: " << strerror(errno) << endl;
    }
}

void Connect(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
    int e = connect(fd, addr, len);
    if (e < 0)
    {
        LOG(ERROR) << "connect error: " << strerror(errno) << endl;
    }
}

// Socket timeout setting mode:

// 1. Call alarm, which generates SIGALARM when the specified timeout expires. This approach involves signal processing,
//    which varies from implementation to implementation and can interfere with existing alarm calls in the process.

// 2. Blocking waits for I/O in SELECT (select has a built-in time limit) instead of directly blocking on read or write calls.

// 3. Use the SO_RECVTIMEO and SO_SNDTIMEO socket options

void ConnectTimeout(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len, uint32_t timeout)
{
    int flags, n, error;
    socklen_t elen;

    // Non-blocking
    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    n = connect(fd, addr, len);
    if (n < 0 && errno != EINPROGRESS)
    {
        LOG(ERROR) << "connectTimeout error: " << strerror(errno) << endl;
    }

    if (n == 0)
    {
        goto done;
    }

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    wset = rset;

    timeval connecttimeout;
    connecttimeout.tv_usec = timeout;

    n = Select(fd + 1, &rset, &wset, nullptr, &connecttimeout);
    if (n == 0)
    {
        close(fd);
        errno = ETIMEDOUT;
        LOG(ERROR) << "connectTimeout error: " << strerror(errno) << endl;
    }

    error = 0;
    if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset))
    {
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &elen) < 0)
            LOG(ERROR) << "connectTimeout error: " << strerror(errno) << endl;
    }

done:
    fcntl(fd, F_SETFL, flags);
    if (error)
    {
        close(fd);
        errno = error;
        LOG(ERROR) << "connectTimeout error: " << strerror(errno) << endl;
    }
}

int Accept(int fd, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len)
{
    int n = accept(fd, addr, addr_len);
    if (n < 0)
    {
        LOG(ERROR) << "accept error: " << strerror(errno) << endl;
    }
    return n;
}

ssize_t Write(int fd, const void *buf, size_t n)
{
    ssize_t num = write(fd, buf, n);
    if (num < 0)
    {
        LOG(ERROR) << "write error: " << strerror(errno) << endl;
    }
    return num;
}

ssize_t Read(int fd, void *buf, size_t n)
{
    ssize_t num = read(fd, buf, n);
    if (num < 0)
    {
        LOG(ERROR) << "read error: " << strerror(errno) << endl;
    }
    return num;
}

void Close(int fd)
{
    int e = close(fd);
    if (e < 0)
    {
        LOG(ERROR) << "close error: " << strerror(errno) << endl;
    }
}

ssize_t Sendto(int fd, const void *buf, size_t n, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len)
{
    ssize_t num = sendto(fd, buf, n, flags, addr, addr_len);
    if (num < 0)
    {
        LOG(ERROR) << "sendto error: " << strerror(errno) << endl;
    }
    return num;
}

ssize_t Recvfrom(int fd, void *__restrict buf, size_t n, int flags, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len)
{
    ssize_t num = recvfrom(fd, buf, n, flags, addr, addr_len);
    if (num < 0)
    {
        LOG(ERROR) << "recvfrom error: " << strerror(errno) << endl;
    }
    return num;
}

int Epoll_create(int size)
{
    int n = epoll_create(size);
    if (n < 0)
    {
        LOG(ERROR) << "epollCreate error: " << strerror(errno) << endl;
    }
    return n;
}

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    int e = epoll_ctl(epfd, op, fd, event);
    if (e < 0)
    {
        LOG(ERROR) << "epollCtl error: " << strerror(errno) << endl;
    }
}

int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    int num = epoll_wait(epfd, events, maxevents, timeout);
    if (num < 0)
    {
        LOG(ERROR) << "epollWait error: " << strerror(errno) << endl;
    }
    return num;
}

void Pipe(int *pipedes)
{
    int e = pipe(pipedes);
    if (e < 0)
    {
        LOG(ERROR) << "pipe error: " << strerror(errno) << endl;
    }
}

void Pipe2(int *pipedes, int flags)
{
    int e = pipe2(pipedes, flags);
    if (e < 0)
    {
        LOG(ERROR) << "pipe2 error: " << strerror(errno) << endl;
    }
}

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    int n = select(nfds, readfds, writefds, exceptfds, timeout);
    if (n < 0)
    {
        LOG(ERROR) << "select error: " << strerror(errno) << endl;
    }
    return n;
}

void Inet_pton(int af, const char *__restrict cp,void *__restrict buf){
    int e=inet_pton(af,cp,buf);
    if(e<0){
        LOG(ERROR) << "inet_pton error: " << strerror(errno) << endl;
    }
}

void Inet_ntop(int af, const void *__restrict cp,char *__restrict buf, socklen_t len){
    //const char* e=inet_ntop(af,cp,buf,len);
    // if(e<0){
    //     LOG(ERROR) << "inet_ntop error: " << strerror(errno) << endl;
    // }
}