#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop)
    : ownerloop_(loop)
{}

bool Poller::HasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->Fd());
    return it != channels_.end() && it->second == channel;
}
