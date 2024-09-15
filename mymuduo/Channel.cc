#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{}

Channel::~Channel() {}

// Channel的Tie方法在什么时候调用 一个TcpConnection新连接创建的时候, TcpConnection ==> Channel
void Channel::Tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
当改变Channel所表示fd的events事件后, update负责在Poller里改变fd相应的事件   epoll_ctl
EventLoop ==> ChannelList   Poller
*/
void Channel::Update()
{
    // 通过Channel所属的EventLoop, 调用Poller的相应方法, 注册fd的events事件
    loop_->UpdateChannel(this);
}

// 在Channel所属的EventLoop中, 把当前的Channel删除
void Channel::Remove()
{
    loop_->RemoveChannel(this);
}

void Channel::HandleEvent(TimeStamp receive_time) // fd得到Poller通知后处理事件
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            HandleEventWithGuard(receive_time);
        }
    }
    else
    {
        HandleEventWithGuard(receive_time);
    }
}

// 根据Poller通知的Channel发生的具体事件, 由Channel负责调用具体的回调操作
void Channel::HandleEventWithGuard(TimeStamp receive_time)
{
    LOG_INFO("Channel HandleEvent revents: %d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closecallback_)
            closecallback_();
    }
    if (revents_ & EPOLLERR)
    {
        if (errorcallback_)
            errorcallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readcallback_)
            readcallback_(receive_time);
    }
    if (revents_ & EPOLLOUT)
    {
        if (writecallback_)
            writecallback_();
    }
}