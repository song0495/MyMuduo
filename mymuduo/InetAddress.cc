#include <strings.h>
#include <arpa/inet.h>
#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    // Convert Internet host address from numbers-and-dots notation in CP
    // into binary data in network byte order.
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

InetAddress::InetAddress(const sockaddr_in& addr)
    :addr_(addr) {}

std::string InetAddress::ToIp() const
{
    char buf[64] = {0};
    // Convert a Internet address in binary network format for interface
    // type AF in buffer starting at CP to presentation form and place
    // result in buffer of length LEN astarting at BUF.
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    
    return buf;
}

std::string InetAddress::ToIpPort() const
{
    // ip:port
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    // buf+end ==> buf指针向后移动end*sizeof(char)位, 即数据末尾
    sprintf(buf+end, ":%u", port);

    return buf;
}

uint16_t InetAddress::ToPort() const
{
    return ntohs(addr_.sin_port);
}

const sockaddr_in* InetAddress::GetSockAddr() const
{
    return &addr_;
}

#include <iostream>
int main()
{
    InetAddress addr(8848);
    std::cout << addr.ToIpPort() << std::endl;
    return 0;
}