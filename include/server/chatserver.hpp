#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
private:
    // 服务器对象
    TcpServer _server;
    // 事件循环对象
    EventLoop *_loop;
    // 响应连接事件的回调函数
    void onConnection(const TcpConnectionPtr &conn);
    // 响应读写事件的回调函数
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time);

public:
    // 构造函数
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);
    // 服务器启动函数
    void start();
};

#endif