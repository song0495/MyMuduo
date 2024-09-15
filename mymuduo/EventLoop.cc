#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

// 防止一个线程创建多个EventLoop thread_local
__thread EventLoop* t_LoopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建WakeupFd, 用来notify唤醒SubReactor处理新来的Channel
int CreateEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error: %d\n", errno);
    }
    
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , CallingPendingFunctors_(false)
    , ThreadId_(CurrentThread::Tid())
    , poller_(Poller::NewDefaultPoller(this))
    , WakeupFd_(CreateEventfd())
    , WakeupChannel_(new Channel(this, WakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, ThreadId_);
    if (t_LoopInThisThread)
    {
        LOG_FATAL("another EventLoop %p exists in this thread %d\n", t_LoopInThisThread, ThreadId_);
    }
    else
    {
        t_LoopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    WakeupChannel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
    // 每一个EventLoop都将监听WakeupChannel的Epoll读事件
    WakeupChannel_->EnableReading();
}

EventLoop::~EventLoop()
{
    WakeupChannel_->DisableAll();
    WakeupChannel_->Remove();
    close(WakeupFd_);
    t_LoopInThisThread = nullptr;
}

void EventLoop::Loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("Eventloop %p start looping\n", this);

    while (!quit_)
    {
        ActiveChannels_.clear();
        // 监听两类fd clit的fd和wakeupfd
        PollReturnTime_ = poller_->Poll(kPollTimeMs, &ActiveChannels_);
        for (Channel* channel : ActiveChannels_)
        {
            // Poller监听哪些Channel发生事件, 然后上报给EventLoop, 通知Channel处理相应事件
            channel->HandleEvent(PollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调函数
        /*
        IO线程 mainloop ==> accept ==> fd ==> channel ==> subloop
        mainloop 事先注册一个回调cb(需要subloop执行) wakeup subloop后, 执行下面的方法, 执行之前mainloop注册的回调函数
        */
        DoPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}

// 退出事件循环 1. loop在自己的线程中调用quit
void EventLoop::Quit()
{
    quit_ = true;

    if (!IsInLoopThread()) // 如果在其他线程中, 调用的quit     在一个subloop中, 调用了mainloop的quit
    {
        Wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::RunInLoop(Functor cb)
{
    if (IsInLoopThread()) // 在当前的loop线程中执行cb
    {
        cb();
    }
    else // 在非当前的loop线程中执行cb, 需要唤醒loop所在线程, 执行cb
    {
        QueneInLoop(cb);
    }
}

// 把cb放入队列中, 唤醒loop所在的线程, 执行cb
void EventLoop::QueneInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        PendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的, 需要执行上面回调操作的loop线程
    // CallingPendingFunctors_: 当前loop正在执行回调, 但是loop又有了新的回调
    if (!IsInLoopThread() || CallingPendingFunctors_)
    {
        Wakeup(); // 唤醒loop所在线程
    }
}

// 唤醒loop所在线程
void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(WakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::Wakeup() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::UpdateChannel(Channel* channel)
{
    poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel)
{
    poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel)
{
    poller_->HasChannel(channel);
}

// 执行回调
void EventLoop::DoPendingFunctors()
{
    std::vector<Functor> functors;
    CallingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(PendingFunctors_);
    }

    for (const Functor& functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调
    }

    CallingPendingFunctors_ = false;
}

void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(WakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::HandleRead() reads %d bytes instead of 8", n);
    }
}