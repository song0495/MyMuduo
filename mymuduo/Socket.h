#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socketfd
class Socket: noncopyable
{
public:
    explicit Socket(int sockfd): sockfd_(sockfd) {}
    ~Socket();

    int Fd() const { return sockfd_; }
    void BindAddress(const InetAddress& localaddr);
    void Listen();
    int Accept(InetAddress* peeraddr);

    void ShutdownWrite(); 

    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);

private:
    const int sockfd_;
};