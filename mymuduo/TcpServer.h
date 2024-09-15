#pragma once

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

// 对外的服务器编程使用的类
class TcpServer: noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop, 
                const InetAddress& listen_addr, 
                const std::string& name_arg, 
                Option optin = kNoReusePort);

    ~TcpServer();

    // 设置底层subloop个数
    void SetThreadNum(int num_threads);

    void SetThreadInitCallback(const ThreadInitCallback& cb) { threadinitcallback_ = cb; }
    void SetConnectionCallback(const ConnectionCallback& cb) { connectioncallback_ = cb; }
    void SetMessageCallback(const MessageCallback& cb) { messagecallback_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback& cb) { writecompletecallback_ = cb; }

    // 开启服务器监听
    void Start();

private:
    void NewConnection(int sockfd, const InetAddress& peer_addr);
    void RemoveConnection(const TcpConnectionPtr& conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_; // BaseLoop 用户定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop, 任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> ThreadPool_; // one loop per thread

    ConnectionCallback connectioncallback_; // 有新连接时的回调
    MessageCallback messagecallback_; // 有读写消息时的回调
    WriteCompleteCallback writecompletecallback_; // 消息发送完成以后的回调

    ThreadInitCallback threadinitcallback_; // loop线程初始化的回调

    std::atomic_int started_;

    int NextConnId_;
    ConnectionMap connections_; // 保存所有的连接
};