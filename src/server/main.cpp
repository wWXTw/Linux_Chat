#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>

void resetHandler(int)
{
    ChatService::instance()->serverClose();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "命令不合法!" << endl;
        exit(-1);
    }
    // 获取端口号和 IP 地址
    std::string ip = argv[1];
    int port = std::atoi(argv[2]);

    signal(SIGINT, resetHandler);
    // 定义服务器变量
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "Linux ChatServer");
    // 启动服务器,开启事件循环
    server.start();
    loop.loop();

    return 0;
}