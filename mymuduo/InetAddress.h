#pragma once 

#include <netinet/in.h>
#include <string>

// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr);

    std::string ToIp() const;
    std::string ToIpPort() const;
    uint16_t ToPort() const;

    const sockaddr_in* GetSockAddr() const;
    void SetSockAddr(const sockaddr_in& addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};