#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

/*
                    EventLoop
        ChannelList             Poller
                        ChannelMap<fd, Channel*>
*/

// Channel未添加到Poller中
const int kNew = -1; // Channel的成员index_= -1
// Channel已添加到Poller中
const int kAdded = 1;
// Channel从Poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }  
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

TimeStamp EpollPoller::Poll(int timeout_ms, ChannelList* active_channels) // epoll_wait
{
    // LOG_DEBUG更为合理
    LOG_INFO("func = %s ==> fd total count: %lu\n", __FUNCTION__, channels_.size());

    int num_events = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);

    int saved_errno = errno;
    TimeStamp now(TimeStamp::now());

    if (num_events > 0)
    {
        LOG_INFO("%d events happened!\n", num_events);
        FillActiveChannels(num_events, active_channels);
        if (num_events == events_.size())
        {
            events_.resize(events_.size() * 2);
        }   
    }
    else if (num_events == 0)
    {
        LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    }
    else
    {
        if (saved_errno != EINTR)
        {
            errno = saved_errno;
            LOG_ERROR("EpollPoller::Poll error!\n");
        }
    }

    return now;
}

// Channel Update Remove ==> EventLoop UpdateChannel RemoveChannel ==> Poller UpdateChannel RemoveChannel
void EpollPoller::UpdateChannel(Channel* channel) // epoll_ctl
{
    const int index = channel->Index();
    LOG_INFO("func = %s ==> fd = %d, events = %d, index = %d\n", __FUNCTION__, channel->Fd(), channel->Events(), index);
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->Fd();
            channels_[fd] = channel;
        }

        channel->SetIndex(kAdded);
        Update(EPOLL_CTL_ADD, channel);
    }
    else // Channel已经在Poller上注册过了
    {
        int fd = channel->Fd();
        if (channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL, channel);
            channel->SetIndex(kDeleted);
        }
        else
        {
            Update(EPOLL_CTL_MOD, channel);
        }
    }    
}

// 从Poller中删除Channel
void EpollPoller::RemoveChannel(Channel* channel) // epoll_ctl
{
    int fd = channel->Fd();
    channels_.erase(fd);

    LOG_INFO("func = %s ==> fd = %d\n", __FUNCTION__, fd);

    int index = channel->Index();
    if (index == kAdded)
    {
        Update(EPOLL_CTL_DEL, channel);
    }
    channel->SetIndex(kNew);
}

// 填写活跃的连接
void EpollPoller::FillActiveChannels(int num_events, ChannelList* active_channels) const
{
    for (int i = 0; i < num_events; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->SetRevents(events_[i].events);
        active_channels->push_back(channel); // EventLoop拿到了它的Poller给它返回的所有发生事件的Channel列表
    }
}

// 更新Channel通道
void EpollPoller::Update(int operation, Channel* channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    int fd = channel->Fd();

    event.events = channel->Events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl delete error: %d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}
