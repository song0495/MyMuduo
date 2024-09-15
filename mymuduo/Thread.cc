#include <semaphore.h>

#include "Thread.h"
#include "CurrentThread.h"

std::atomic_int Thread::NumCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    SetDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
    
}

void Thread::Start() // 一个Thread对象, 记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid值
        tid_ = CurrentThread::Tid();
        sem_post(&sem);
        // 开启一个新线程, 专门执行该线程函数
        func_();
    }));
 
    // 这里必须等待获取上面创建的线程的tid值
    sem_wait(&sem);
}

void Thread::Join()
{
    joined_ = true;
    thread_->join();
}

void Thread::SetDefaultName()
{
    int num = ++NumCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}