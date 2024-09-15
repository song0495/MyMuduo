#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"

class EpollPoller: public Poller
{
public:
    EpollPoller(EventLoop* loop); // epoll_create
    ~EpollPoller() override;

    // 重写基类Poller的抽象方法
    TimeStamp Poll(int timeout_ms, ChannelList* active_channels) override; // epoll_wait
    void UpdateChannel(Channel* channel) override; // epoll_ctl
    void RemoveChannel(Channel* channel) override; // epoll_ctl

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void FillActiveChannels(int num_events, ChannelList* active_channels) const;
    // 更新Channel通道
    void Update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};