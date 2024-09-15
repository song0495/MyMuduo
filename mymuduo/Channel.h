#pragma once

#include <memory>
#include <functional>

#include "noncopyable.h"
#include "Timestamp.h"

// EventLoop类型的前置声明, 只在.h文件中使用, 具体实现的使用在.cc文件中
// 由于头文件需要向外部提供, 避免暴露过多头文件
class EventLoop;

/*
理清EventLoop Channel Poller之间的关系 <== Reactor模型上对应Demultiplex
Channel理解为通道, 封装了sockfd和其感兴趣的event, 如EPOLLIN EPOLLOUT事件
还绑定了Poller返回的具体事件
*/
class Channel: noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    // 使用EventLoop*, 大小是确定的, 可以使用类型前置声明
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 使用了具体的类型, 无法使用类型前置声明, 需要引入头文件
    void HandleEvent(TimeStamp receive_time); // fd得到Poller通知后处理事件

    // 设置回调函数对象
    void SetReadCallback(ReadEventCallback cb) { readcallback_ = std::move(cb); }
    void SetWriteCallback(EventCallback cb) { writecallback_ = std::move(cb); }
    void SetCloseCallback(EventCallback cb) { closecallback_ = std::move(cb); }
    void SetErrorCallback(EventCallback cb) { errorcallback_ = std::move(cb); }

    // 防止当Channel被手动remove掉, Channel还在执行回调操作
    void Tie(const std::shared_ptr<void>&);

    int Fd() const { return fd_; }
    int Events() const { return events_; }
    void SetRevents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void EnableReading() { events_ |= kReadEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    // 返回fd当前的事件状态
    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    bool IsReading() const { return events_ & kReadEvent; }

    int Index() { return index_; }
    void SetIndex(int idx) { index_ = idx; }

    EventLoop* OwnerLoop() { return loop_; }
    void Remove();
    
private:
    void Update();
    void HandleEventWithGuard(TimeStamp receive_time);

    // 感兴趣事件的状态描述
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; // 事件循环
    const int fd_; // fd, Poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_; // Poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为Channel通道里面能够获知fd最终发生的具体事件revents, 所以它负责调用具体事件的回调操作
    ReadEventCallback readcallback_;
    EventCallback writecallback_;
    EventCallback closecallback_;
    EventCallback errorcallback_;
};