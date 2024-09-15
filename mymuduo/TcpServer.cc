#include <functional>
#include <strings.h>

#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s: %s: %d MainLoop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }

    return loop;
}

TcpServer::TcpServer(EventLoop* loop, 
                    const InetAddress& listen_addr, 
                    const std::string& name_arg, 
                    Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listen_addr.ToIpPort())
    , name_(name_arg)
    , acceptor_(new Acceptor(loop, listen_addr, option == kReusePort))
    , ThreadPool_(new EventLoopThreadPool(loop, name_))
    , connectioncallback_()
    , messagecallback_()
    , NextConnId_(1)
    , started_(0)
{
    // 当有新用户连接时, 会执行TcpServer::NewConnection回调 
    acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, 
                                        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto& item : connections_)
    {
        // 这个局部的shared_ptr对象, 出右括号, 可以自动释放new出来的TcpConnection对象资源
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        // 销毁连接
        conn->GetLoop()->RunInLoop(std::bind(&TcpConnection::ConnectDestoryed, conn));
    }
}

// 设置底层subloop个数
void TcpServer::SetThreadNum(int num_threads)
{
    ThreadPool_->SetThreadNum(num_threads);
}

// 开启服务器监听
void TcpServer::Start()
{
    if (started_++ == 0) // 防止一个TcpServer对象被启动多次
    {
        ThreadPool_->Start(threadinitcallback_); // 启动底层的loop线程池
        loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
    }
    
}

// 有一个新的客户端连接, accpetpr会执行这个回调
void TcpServer::NewConnection(int sockfd, const InetAddress& peer_addr)
{
    // 轮询算法, 选择一个subloop来管理Channel
    EventLoop* io_loop = ThreadPool_->GetNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), NextConnId_);
    std::string conn_name = name_ + buf;

    LOG_INFO("TcpServer::NewConnection [%s] - new connection [%s] from %s\n", 
            name_.c_str(), conn_name.c_str(), peer_addr.ToIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机IP和port
    sockaddr_in local;
    bzero(&local, sizeof(local));
    socklen_t addr_len = sizeof(local);
    if (getsockname(sockfd, (sockaddr*)&local, &addr_len) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress local_addr(local);

    // 根据连接成功的sockfd, 创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(io_loop, conn_name, sockfd, local_addr, peer_addr));
    connections_[conn_name] = conn;
    // 下面的回调都是用户设置给TcpServer ==> TcpConnection ==> Channel ==> Poller ==> notify Channel调用回调
    conn->SetConnectionCallback(connectioncallback_);
    conn->SetMessageCallback(messagecallback_);
    conn->SetWriteCompleteCallback(writecompletecallback_);

    // 设置了如何关闭连接的回调
    conn->SetCloseCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));

    // 直接调用TcpConnection::ConnectEstablished
    io_loop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
{
    loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::RemoveConnectionInLoop [%s] - connection %s\n", 
            name_.c_str(), conn->Name().c_str());
    
    size_t n = connections_.erase(conn->Name());
    EventLoop* io_loop = conn->GetLoop();
    io_loop->QueneInLoop(std::bind(&TcpConnection::ConnectDestoryed, conn));
}