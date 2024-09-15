#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor: noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuseport);
    ~Acceptor();

    void SetNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newconnectioncallback_ = std::move(cb);
    }

    bool Listenning() const { return listenning_; }
    void Listen();

private:
    void HandleRead();

    EventLoop* loop_; // Acceptor用的就是用户定义的那个BaseLoop, 也叫mainloop
    Socket AcceptSocket_;
    Channel AcceptChannel_;
    NewConnectionCallback newconnectioncallback_;
    bool listenning_;
};