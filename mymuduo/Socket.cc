#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::BindAddress(const InetAddress& localaddr)
{
    if (0 != bind(sockfd_, (sockaddr*)localaddr.GetSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind socket: %d fail\n", sockfd_);
    }
}

void Socket::Listen()
{
    if (0 != listen(sockfd_, 1024))
    {
        LOG_FATAL("listen socket: %d fail\n", sockfd_);
    }
}

int Socket::Accept(InetAddress* peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    int connfd = accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd > 0)
    {
        peeraddr->SetSockAddr(addr);
    }
    
    return connfd;
}

void Socket::ShutdownWrite()
{
    if (shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("ShutdownWrite error");
    }
}

void Socket::SetTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::SetReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::SetReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::SetKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}