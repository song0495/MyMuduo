#include <stdlib.h>

#include "Poller.h"
#include "EpollPoller.h"

/*
为什么要额外写一个DefaultPoller.cc来实现NewDefaultPoller方法
如果在Poller.cc中实现该方法, 为了返回poll和epoll实例, 那么必然会引入
PollPoller.h和EpollPoller.h, 在基类实现中引用派生类头文件, 这是不合适的
*/

Poller* Poller::NewDefaultPoller(EventLoop* loop)
{
    if (getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // poll实例
    }
    else
    {
        return new EpollPoller(loop); // epoll实例
    } 
}