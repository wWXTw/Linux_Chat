#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

using namespace placeholders;

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 绑定消息与回调函数 (解耦)
    _handler.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _handler.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _handler.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _handler.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _handler.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _handler.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _handler.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    _handler.insert({LOGOUT_MSG, bind(&ChatService::logout, this, _1, _2, _3)});

    // // 连接Redis服务器
    // if (_redis.connect())
    // {
    //     // 设置上报信息的回调
    //     _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    // }

    // 连接Kafka中间件
    if (_kafka.connect())
    {
        // 设置回调
        _kafka.init_notify_handler(bind(&ChatService::handleKafkaMessage, this, _1, _2));
    }
}

MsgHandler ChatService::getHandler(int msg)
{
    auto it = _handler.find(msg);
    if (it == _handler.end())
    {
        // 返回默认的处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            // 将没有针对当前消息的处理器的情况写入错误日志
            LOG_ERROR << "MsgId: " << msg << " can't find its handler";
        };
    }
    else
    {
        return _handler[msg];
    }
}

void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];

    {
        unique_lock<mutex> g1(_mtx);
        auto it = _userConn.find(userid);
        if (it != _userConn.end())
        {
            _userConn.erase(it);
        }
    }

    // 修改数据库中的状态
    User user;
    user.setId(userid);
    user.setState("offline");
    _user.updateState(user);

    // // 从Redis中取消订阅
    // _redis.unsubscribe(userid);

    // 取消订阅Topic
    string topic = to_string(userid);
    _kafka.unsubscribe(topic);
}

void ChatService::clientClose(const TcpConnectionPtr &conn)
{
    User user;
    cout << "external" << endl;
    // 通过conn获取用户Id (注意线程安全)
    {
        unique_lock<mutex> g1(_mtx);
        // 遍历MAP表过于繁琐
        // unordered_map反向映射...
        for (auto it = _userConn.begin(); it != _userConn.end(); it++)
        {
            if (it->second == conn)
            {
                cout << "internal" << endl;
                // 从MAP表中删除连接信息
                user.setId(it->first);
                _userConn.erase(it);

                // 修改数据库中的状态
                user.setState("offline");
                _user.updateState(user);

                // // 从Redis中取消订阅
                // _redis.unsubscribe(user.getId());

                string topic = to_string(user.getId());
                _kafka.unsubscribe(topic);

                break;
            }
        }
    }
}

void ChatService::serverClose()
{
    _user.resetState();
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    cout << "test_login" << endl;

    string name = js["name"];
    string password = js["password"];
    User result = _user.queryUserName(name);
    if (result.getPassword() == password)
    {
        if (result.getState() == "online")
        {
            // 该账号已经登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errorid"] = 2;
            response["errortext"] = "该账号已在别处登录";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功
            // 用户state状态更新
            result.setState("online");
            _user.updateState(result);

            // 记录用户连接信息 (保障线程安全)
            {
                unique_lock<mutex> g1(_mtx);
                _userConn.insert({result.getId(), conn});
            }

            // // 向Redis订阅通道
            // _redis.subscribe(result.getId());

            string topic = to_string(result.getId());
            _kafka.subscribe(topic);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errorid"] = 0;
            response["id"] = result.getId();
            response["name"] = result.getName();

            // 先向客户端发送一个登录成功的消息
            conn->send(response.dump());

            // 获取ID号
            int userid = result.getId();
            // 异步获取好友信息,群组信息与离线消息
            thread([this, conn, userid]()
                   {
                    json response_info;
                    response_info["msgid"] = LOGIN_INFO_ACK;
                    // 查询是否有离线消息
                    unordered_map<string, vector<string>> offlineMsg = _offline.query(userid);
                    if (!offlineMsg.empty())
                    {
                        response_info["offlineMsg"] = offlineMsg;
                        _offline.remove(userid);
                    }

                    // 查询好友信息
                    vector<User> friends = _friend.query(userid);
                    if (!friends.empty())
                    {
                        vector<string> fri;
                        for (User &user : friends)
                        {
                            json temp;
                            temp["id"] = user.getId();
                            temp["name"] = user.getName();
                            temp["state"] = user.getState();
                            fri.push_back(temp.dump());
                        }
                        response_info["friends"] = fri;
                    }

                    // 查询群组信息
                    vector<Group> group = _group.queryGroup(userid);
                    if (!group.empty())
                    {
                        vector<string> gro;
                        for (Group &g : group)
                        {
                            json temp;
                            temp["groupid"] = g.getId();
                            temp["name"] = g.getName();
                            temp["desc"] = g.getDesc();
                            vector<GroupUser> user_vec = g.getUsers();
                            vector<string> gro_user;
                            // 发送群组成员
                            for (auto &guser : user_vec)
                            {
                                json mem;
                                mem["guserid"] = guser.getId();
                                mem["gusername"] = guser.getName();
                                mem["guserrole"] = guser.getRole();
                                mem["guserstate"] = guser.getState();
                                gro_user.push_back(mem.dump());
                            }
                            temp["users"] = gro_user;
                            gro.push_back(temp.dump());
                        }
                        response_info["group"] = gro;
                    }
                    conn->send(response_info.dump()); })
                .detach();
        }
    }
    else
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errorid"] = 1;
        response["errortext"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    User user;
    user.setName(js["name"]);
    user.setPassword(js["password"]);
    bool state = _user.insertUser(user);

    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errorid"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errorid"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    int from = js["from"].get<int>();
    int to = js["to"].get<int>();
    string msg = js["msg"];
    // 查看接受者当前是否在线
    {
        unique_lock<mutex> g1(_mtx);
        auto it = _userConn.find(to);
        if (it != _userConn.end())
        {
            // 接收者在线
            it->second->send(js.dump());
            return;
        }
    }
    // 查询接收者是否在别的服务器上
    User user = _user.queryUserId(to);
    if (user.getState() == "offline")
    {
        // 接收者不在线,存储离线消息
        _offline.insert(to, msg, name);
    }
    else
    {
        // // 接收者在别的服务器上,发送Pulish请求
        // _redis.publish(to, js.dump());

        string topic = to_string(to);
        string jsmsg = js.dump();
        _kafka.publish(topic, jsmsg);
    }
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    _friend.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    Group group(-1, name, desc);
    if (_group.createGroup(group))
    {
        // 创建成功将userid存入,记为创建者
        _group.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 以普通用户的身份加入群组
    _group.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    string msg = js["msg"];
    vector<int> member = _group.queryGroupUsers(userid, groupid);
    // 对每个群组内的其他成员发送消息 (线程安全)
    for (int id : member)
    {
        unique_lock<mutex> g1(_mtx);
        auto it = _userConn.find(id);
        if (it != _userConn.end())
        {
            // 群友在线直接发送
            it->second->send(js.dump());
            continue;
        }
        // 查询群友是否在别的服务器上
        User user = _user.queryUserId(id);
        if (user.getState() == "offline")
        {
            // 群友不在线,存储离线消息
            _offline.insert(id, msg, name);
        }
        else
        {
            // // 群友在别的服务器上,发送Pulish请求
            // _redis.publish(id, js.dump());

            string topic = to_string(id);
            string jsmsg = js.dump();
            _kafka.publish(topic, jsmsg);
        }
    }
}

// void ChatService::handleRedisSubscribeMessage(int userid, string msg)
// {
//     unique_lock<mutex> g1(_mtx);
//     auto it = _userConn.find(userid);
//     if (it != _userConn.end())
//     {
//         it->second->send(msg);
//         return;
//     }

//     json js = json::parse(msg);
//     string message = js["msg"];
//     string name = js["name"];
//     _offline.insert(userid, msg, name);
// }

void ChatService::handleKafkaMessage(string topic, string msg)
{
    cout << "Ready here" << endl;
    int userid = stoi(topic);

    {
        unique_lock<mutex> g1(_mtx);
        auto it = _userConn.find(userid);
        if (it != _userConn.end())
        {
            it->second->send(msg);
            return;
        }
    }

    json js = json::parse(msg);
    string message = js["msg"];
    string name = js["name"];
    _offline.insert(userid, msg, name);
}