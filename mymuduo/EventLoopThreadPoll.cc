#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const std::string& name_arg)
    : BaseLoop_(baseloop)
    , name_(name_arg)
    , started_(false)
    , NumThread_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::Start(const ThreadInitCallback& cb)
{
    started_ = true;

    for (int i = 0; i < NumThread_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->StartLoop()); // 底层创建线程, 绑定一个新的EventLoop, 并返回该loop的地址
    }
    
    // 整个服务器只有一个线程, 运行着BaseLoop
    if (NumThread_ == 0 && cb)
    {
        cb(BaseLoop_);
    }
}

// 如果工作在多线程中, BaseLoop_默认以轮询的方式分配Channel给subloop
EventLoop* EventLoopThreadPool::GetNextLoop()
{
    EventLoop* loop = BaseLoop_;

    if (!loops_.empty()) // 通过轮询获取下一个处理时间的loop
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }
    
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, BaseLoop_);
    }
    else
    {
        return loops_;
    }
}