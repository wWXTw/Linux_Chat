#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include <string>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "kafkaservice.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 服务器业务模块--单例
class ChatService
{
private:
    // 构造函数
    ChatService();

    // 用于选择回调函数的map
    unordered_map<int, MsgHandler> _handler;

    // 互斥锁
    mutex _mtx;

    // // Redis服务对象
    // Redis _redis;

    // Kafka服务对象
    KafkaService _kafka;

    // 用户表处理器
    UserModel _user;

    // 离线消息表处理器
    OfflineMessageModel _offline;

    // 好友表处理器
    FriendModel _friend;

    // 群组表处理器
    GroupModel _group;

    // 用户连接信息存储器
    unordered_map<int, TcpConnectionPtr> _userConn;

public:
    // 获取单例对象的接口函数
    static ChatService *instance();

    // 根据消息返回对应处理器
    MsgHandler getHandler(int msg);

    // 处理客户异常退出
    void clientClose(const TcpConnectionPtr &conn);

    // 处理服务器退出
    void serverClose();

    // 登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群聊业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 用户退出登录业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // // 从redis消息队列中获取订阅的消息
    // void handleRedisSubscribeMessage(int userid, string msg);

    // 从Kafka消息队列中获取订阅的消息
    void handleKafkaMessage(string topic, string msg);
};

#endif