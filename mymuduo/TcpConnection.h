#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;

/*
TcpServer ==> Acceptor ==> 有一个新用户连接, 通过accept函数拿到connfd
==> TcpConnection 设置回调 ==> Channel ==> Poller ==> Channel回调操作
*/

class TcpConnection: noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, 
                    const std::string& name_arg, 
                    int sockfd, 
                    const InetAddress& local_addr, 
                    const InetAddress& peer_addr);
    ~TcpConnection();

    EventLoop* GetLoop() const { return loop_; }
    const std::string& Name() const {return name_; }
    const InetAddress& LocalAddress() const { return LocalAddr_; }
    const InetAddress& PeerAddress() const { return PeerAddr_; }

    bool Connected() const { return state_ == kConnected; }
    bool Disconnected() const { return state_ == kDisconnected; }

    // 发送数据
    void Send(const std::string& buf);
    // 关闭连接
    void Shutdown();

    void SetConnectionCallback(const ConnectionCallback& cb) { connectioncallback_ = cb; }
    void SetMessageCallback(const MessageCallback& cb) { messagecallback_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback& cb) { writecompletecallback_ = cb; }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb) { highwatermarkcallback_ = cb; }
    void SetCloseCallback(const CloseCallback& cb) { closecallback_ = cb; }

    void ConnectEstablished();
    void ConnectDestoryed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};

    void SetState(StateE state) { state_ = state; }

    void HandleRead(TimeStamp reveive_time);
    void HandleWrite();
    void HandleClose();
    void HandleError();

    void SendInLoop(const void* data, size_t len);
    void ShutdownInLoop();

    EventLoop* loop_; // 这里绝对不是BaseLoop, 因为TcpConnection都是在subloop里面管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress LocalAddr_;
    const InetAddress PeerAddr_;

    ConnectionCallback connectioncallback_; // 有新连接时的回调
    MessageCallback messagecallback_; // 有读写消息时的回调
    WriteCompleteCallback writecompletecallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback highwatermarkcallback_;
    CloseCallback closecallback_;

    size_t HighWaterMark_;

    Buffer InputBuffer_;
    Buffer OutputBuffer_;
};