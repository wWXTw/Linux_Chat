#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{
private:
    // hiredis同步上下文对象 -- publish
    redisContext *_publish_context;

    // hiredis同步上下文对象 -- subsribe
    redisContext *_subscribe_context;

    // 回调操作,收到订阅的消息,给service层上报
    function<void(int, string)> _notify_handler;

public:
    Redis();
    ~Redis();

    // 连接Redis服务器
    bool connect();

    // 向Redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向Redis指定的通道channel订阅消息
    bool subscribe(int channel);

    // 向Redis指定的通道channel取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道的消息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);
};

#endif