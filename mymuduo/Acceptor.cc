#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

static int CreateNonblocking()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("listen socket create error: %d\n", errno);
    }   
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuseport)
    : loop_(loop)
    , AcceptSocket_(CreateNonblocking())
    , AcceptChannel_(loop, AcceptSocket_.Fd())
    , listenning_(false)
{
    AcceptSocket_.SetReuseAddr(true);
    AcceptSocket_.SetReusePort(true);
    AcceptSocket_.BindAddress(listen_addr);
    // 有新用户的连接, 要执行一个回调 (connfd ==> channel ==> subloop)
    AcceptChannel_.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor()
{
    AcceptChannel_.DisableAll();
    AcceptChannel_.Remove();
}

void Acceptor::Listen()
{
    listenning_ = true;
    AcceptSocket_.Listen(); // listen
    AcceptChannel_.EnableReading(); //  AcceptChannel_ ==> Poller
}

// listenfd有事件发生, 就是有新用户连接
void Acceptor::HandleRead()
{
    InetAddress peer_addr;
    int connfd = AcceptSocket_.Accept(&peer_addr);
    if (connfd >= 0)
    {
        if (newconnectioncallback_)
        {
            newconnectioncallback_(connfd, peer_addr); // 轮询找到subloop, 唤醒, 分发当前的新客户端的Channel
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("accept error: %d\n", errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("sockfd reached limit!\n");
        }
    }
}