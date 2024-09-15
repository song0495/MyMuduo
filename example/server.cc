#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : server_(loop, addr, name), loop_(loop)
    {
        // 注册回调
        server_.SetConnectionCallback(std::bind(&EchoServer::OnConn, this, std::placeholders::_1));

        server_.SetMessageCallback(std::bind(&EchoServer::OnMsg, this, 
                                    std::placeholders::_1, 
                                    std::placeholders::_2, 
                                    std::placeholders::_3));

        // 设置合适的loop线程数
        server_.SetThreadNum(3);
    }

    void Start()
    {
        server_.Start();
    }

private:
    void OnConn(const TcpConnectionPtr& conn)
    {
        if (conn->Connected())
        {
            LOG_INFO("conn up : %s\n", conn->PeerAddress().ToIpPort().c_str());
        } 
        else
        {
            LOG_INFO("conn down : %s\n", conn->PeerAddress().ToIpPort().c_str());
        }  
    }

    void OnMsg(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time)
    {
        std::string msg = buf->RetrieveAllAsString();
        conn->Send(msg);
        conn->Shutdown(); // 写端 EPOLLHUP ==> closecallback_
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(9527);
    EchoServer server(&loop, addr, "EchoServer");
    server.Start();
    loop.Loop();

    return 0;
}