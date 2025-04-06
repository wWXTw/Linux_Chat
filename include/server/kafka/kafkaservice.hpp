#ifndef KAFKA_SERVICE_H
#define KAFKA_SERVICE_H

#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <librdkafka/rdkafkacpp.h>

using namespace std;
using namespace RdKafka;

class KafkaService
{
public:
    KafkaService();
    ~KafkaService();

    // 启动函数
    bool connect();

    // 发送消息的函数
    bool publish(string &topic, string &msg);

    // 订阅主题的函数
    void subscribe(string &topic);

    // 取消订阅主题的函数
    void unsubscribe(string &topic);

    // 更新消费者订阅主题的函数
    void update();

    // 结束服务的函数
    void close();

    // 设置回调函数的函数
    void init_notify_handler(function<void(string, string)> fn);

private:
    // 生产者
    Producer *_producer;

    // 消费者
    KafkaConsumer *_consumer;

    // Topic集合
    unordered_set<string> _topics;

    // 接收消息的函数
    void consume_loop();

    // 接收消息的线程
    thread consume_thread;

    // 接收到消息的回调函数
    function<void(string, string)> _notify_handler;

    // 互斥锁
    mutex mtx;

    // 更新状态的原子变量
    atomic<bool> _updating;

    // 服务运行的原子变量
    atomic<bool> _running;
};

#endif