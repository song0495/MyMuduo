#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool: noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseloop, const std::string& name_arg);
    ~EventLoopThreadPool();

    void SetThreadNum(int num_threads) { NumThread_ = num_threads; }

    void Start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 如果工作在多线程中, BaseLoop_默认以轮询的方式分配Channel给subloop
    EventLoop* GetNextLoop();

    std::vector<EventLoop*> GetAllLoops();

    bool Started() const { return started_; }

    const std::string& Name() const { return name_; }

private:
    EventLoop* BaseLoop_;
    std::string name_;
    bool started_;
    int NumThread_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>>threads_;
    std::vector<EventLoop*> loops_;
};