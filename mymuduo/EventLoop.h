#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类, 主要包含了两大模块 Channel Poller(epoll的抽象)
class EventLoop: noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启/退出事件循环
    void Loop();
    void Quit();

    TimeStamp PollReturnTime() const { return PollReturnTime_; }

    // 在当前loop中执行cb
    void RunInLoop(Functor cb);
    // 把cb放入队列中, 唤醒loop所在的线程, 执行cb
    void QueneInLoop(Functor cb);

    // 唤醒loop所在线程
    void Wakeup();

    // EventLoop的方法 ==> Poller的方法
    void UpdateChannel(Channel* channel);
    void RemoveChannel(Channel* channel);
    bool HasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool IsInLoopThread() const { return ThreadId_ == CurrentThread::Tid(); }

private:
    void HandleRead(); // Wakeup
    void DoPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作, 通过CAS实现
    std::atomic_bool quit_; // 标识退出loop循环
    
    const pid_t ThreadId_; // 记录当前loop所在的线程id

    TimeStamp PollReturnTime_; // Poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // 主要作用: 当mainloop获取一个新用户的Channel, 通过轮询算法选择一个subloop, 通过该成员唤醒subloop处理Channel
    int WakeupFd_; 
    std::unique_ptr<Channel> WakeupChannel_;

    ChannelList ActiveChannels_;

    std::atomic_bool CallingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> PendingFunctors_; // 存储loop需要执行的所有回调操作
    
    std::mutex mutex_; // 互斥锁, 用来保护上面vector容器的线程安全操作
};