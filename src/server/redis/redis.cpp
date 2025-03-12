#include "redis.hpp"
#include <iostream>

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "连接Redis数据库失败" << endl;
        return false;
    }

    // 负责subscribe发布消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "连接Redis数据库失败" << endl;
        return false;
    }

    // 在线程中监听通道上的事件,有消息上报给事务层
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "连接Redis数据库成功" << endl;
    return true;
}

bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "发布请求失败!" << endl;
        return false;
    }
    cout << channel << endl;
    cout << "test_publish_success" << endl;
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    if (redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "订阅请求失败!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done) != REDIS_OK)
        {
            cerr << "订阅请求失败!" << endl;
            return false;
        }
    }

    cout << "test_subscribe" << channel << endl;

    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "取消订阅请求失败!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "取消订阅请求失败!" << endl;
            return false;
        }
    }

    return true;
}

void Redis::observer_channel_message()
{
    cout << "start" << endl;
    redisReply *reply = nullptr;
    while (redisGetReply(this->_subscribe_context, (void **)&reply) == REDIS_OK)
    {
        cout << "test_reply_1" << endl;
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            cout << "test_reply_2" << endl;
            cout << reply->element[2]->str << endl;
            // 订阅收到的消息不为空
            _notify_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
    }
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_handler = fn;
}