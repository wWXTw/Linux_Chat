#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 给服务器注册用户连接的创建与断开的回调函数
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    // 给服务器注册用户读写事件的回调函数
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务器的线程
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        // 上线信息
        cout << conn->peerAddress().toIpPort()
             << "->" << conn->localAddress().toIpPort() << " state:online" << endl;
    }
    else
    {
        // 下线信息
        cout << conn->peerAddress().toIpPort()
             << "->" << conn->localAddress().toIpPort() << " state:offline" << endl;
        // 处理客户下线
        ChatService::instance()->clientClose(conn);
        // 释放socket等资源
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 反序列化 字符串->JSON
    json js = json::parse(buf);
    // 通过JSON中的MsgId获取处理器并执行
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
}