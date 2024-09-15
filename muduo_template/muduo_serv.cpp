/*
muduo网络库给用户提供了两个主要的类
TcpServer: 用于编写服务器程序
TcpClient: 用于编写客户端程序

epoll + 线程池
好处: 能够把网络I/O的代码和业务代码分开
    用户的连接与断开 <===> 用户的可读写事件
*/

#include <iostream>
#include <string>
#include <functional>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

/*
基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数, 输出Server的构造函数
4. 在当前服务器类的构造函数中, 注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务器端线程数量, muduo库会自己分配I/O线程和Worker线程
*/

class Server
{
public:
    Server(muduo::net::EventLoop* loop, // 事件循环
            const muduo::net::InetAddress& listenAddr, // IP + Port
            const std::string& nameArg) // 服务器的名字
            : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&Server::OnConnection, 
                                                this, std::placeholders::_1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&Server::OnMessage, this, 
                                            std::placeholders::_1, 
                                            std::placeholders::_2, 
                                            std::placeholders::_3));

        // 设置服务器端的线程数量   1个I/O线程 3个Worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void Start()
    {
        _server.start();
    }

private:
    muduo::net::TcpServer _server;
    muduo::net::EventLoop* _loop;

    // 专门处理用户的连接创建和断开    epoll listenfd accept
    void OnConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << 
                        conn->localAddress().toIpPort() << " state: on" << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << 
                        conn->localAddress().toIpPort() << " state: off" << std::endl;
            conn->shutdown();
        }
    }
    // 专门处理用户的读写事件
    void OnMessage(const muduo::net::TcpConnectionPtr& conn, // 连接
                    muduo::net::Buffer* buffer, // 缓冲区
                    muduo::Timestamp time) // 接收到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data: " << buf << " time: " << time.toString() << std::endl;
        conn->send(buf);
    }
};

int main()
{
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 9527);
    Server server(&loop, addr, "server");

    server.Start(); // listenfd epoll_ctl ==> epoll
    loop.loop(); // epoll_wait 以阻塞方式等待新用户连接和已连接用户的读写事件

    return 0;
}