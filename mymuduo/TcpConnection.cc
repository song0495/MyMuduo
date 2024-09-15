#include <functional>

#include <errno.h>

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s: %s: %d TcpConnection Loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }

    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, 
                    const std::string& name_arg, 
                    int sockfd, 
                    const InetAddress& local_addr, 
                    const InetAddress& peer_addr)
    : loop_(CheckLoopNotNull(loop))
    , name_(name_arg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , LocalAddr_(local_addr)
    , PeerAddr_(peer_addr)
    , HighWaterMark_(64*1024*1024) // 64M
{
    // 给Channel设置相应的回调函数, Poller给Channel通知感兴趣的事件发生, channel会调用相应的操作函数
    channel_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this, 
                                        std::placeholders::_1));

    channel_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));

    channel_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));

    channel_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state = %d\n", name_.c_str(), channel_->Fd(), (int)state_);
}

void TcpConnection::Send(const std::string& buf)
{
    if (state_ == kConnected)
    {
        if (loop_->IsInLoopThread())
        {
            SendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->RunInLoop(std::bind(&TcpConnection::SendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::Shutdown()
{
    if (state_ == kConnected)
    {
        SetState(kDisconnecting);
        loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
    }
}

void TcpConnection::ConnectEstablished()
{
    SetState(kConnected);
    channel_->Tie(shared_from_this());
    channel_->EnableReading(); // 向Poller注册Channel的epollin事件

    // 新连接建立, 执行回调
    connectioncallback_(shared_from_this());
}

void TcpConnection::ConnectDestoryed()
{
    if (state_ == kConnected)
    {
        SetState(kDisconnected);
        channel_->DisableAll(); // 把Channel的所有感兴趣事件从Poller中del
    }
    channel_->Remove(); // 把Channel从Poller中del
}

void TcpConnection::HandleRead(TimeStamp reveive_time)
{
    int saved_errno = 0;
    ssize_t n = InputBuffer_.ReadFd(channel_->Fd(), &saved_errno);
    if (n > 0)
    {
        // 已建立连接的用户, 有可读事件发生了, 调用用户传入的回调操作OnMessage
        messagecallback_(shared_from_this(), &InputBuffer_, reveive_time);
    }
    else if (n == 0)
    {
        HandleClose();
    }
    else
    {
        errno = saved_errno;
        LOG_ERROR("TcpConnection::HandleRead error\n");
        HandleError();
    }
}

void TcpConnection::HandleWrite()
{
    if (channel_->IsWriting())
    {
        int saved_errno = 0;
        ssize_t n = OutputBuffer_.WriteFd(channel_->Fd(), &saved_errno);
        if (n > 0)
        {
            OutputBuffer_.Retrieve(n);
            if (OutputBuffer_.ReadableBytes() == 0)
            {
                channel_->DisableWriting();
                if (writecompletecallback_)
                {
                    loop_->QueneInLoop(std::bind(writecompletecallback_, shared_from_this()));
                }   
                if (state_ == kDisconnecting)
                {
                    ShutdownInLoop();
                }
            } 
        }
        else
        {
            LOG_ERROR("TcpConnection::HandleWrite error");
        }
    }
   else
   {
        LOG_ERROR("TcpConnection fd = %d is down, no more writing\n", channel_->Fd());
   } 
}

// Poller ==> Channel::closecallback ==> TcpConnection::HandleClose
void TcpConnection::HandleClose()
{
    LOG_INFO("TcpConnection::HandleClose error, fd = %d state = %d\n", channel_->Fd(), (int)state_);
    SetState(kDisconnected);
    channel_->DisableAll();

    TcpConnectionPtr ConnPtr(shared_from_this());
    connectioncallback_(ConnPtr); // 执行连接关闭的回调
    closecallback_(ConnPtr); // 关闭连接的回调 执行的是TcpServer::RemoveConnection回调方法
}

void TcpConnection::HandleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int errnum = 0;
    if (getsockopt(channel_->Fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        errnum = errno;
    }
    else
    {
        errnum = optval;
    }
    LOG_ERROR("TcpConnection::HandleError error, name: %s - SO_ERROR: %d\n", name_.c_str(), errnum);
}

// 发送数据: 应用写的快, 而内核发送数据慢, 需要把待发送数据写入缓冲区, 而且设置了水位回调
void TcpConnection::SendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;

    // 之前调用过该connection的shutdown, 不能再发送
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing\n");
        return;
    }
    //  表示channel——第一次开始写数据, 而且缓冲区没有待发送数据
    if (!channel_->IsWriting() && OutputBuffer_.ReadableBytes() == 0)
    {
        nwrote = write(channel_->Fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writecompletecallback_)
            {
                // 既然在这里数据全部发送完成, 就不用再给Channel设置epollout事件了
                loop_->QueneInLoop(std::bind(writecompletecallback_, shared_from_this()));
            } 
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::SendInLoop error");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    fault_error = true;
                }
            }
        } 
    }
    // 说明这一次write没有把数据全部发送出去, 剩余的数据需要保存到缓冲区中, 然后给Channel注册
    // epollout事件, Poller发现tcp的发送缓冲区有空间, 会通知相应的sock——channel, 调用writecallback_方法
    // 也就是调用TcpConnection::HandleWrite方法, 把发送缓冲区中的数据全部发送完成
    if (!fault_error && remaining > 0)
    {
        size_t old_len = OutputBuffer_.ReadableBytes();
        if (old_len + remaining <= HighWaterMark_ && old_len < HighWaterMark_ && highwatermarkcallback_)
        {
            loop_->QueneInLoop(std::bind(highwatermarkcallback_, shared_from_this(), old_len+remaining));
        }
        OutputBuffer_.Append((char*)data + nwrote, remaining);
        if (!channel_->IsWriting())
        {
            channel_->EnableWriting(); // 这里一定要注册Channel的写事件, 否则Poller不会给Channel通知epollout
        }
    }
}

void TcpConnection::ShutdownInLoop()
{
   if (!channel_->IsWriting()) // 说明OutputBuffer_中的数据已经全部发送完成
   {
        socket_->ShutdownWrite(); // 关闭写端
   }
}