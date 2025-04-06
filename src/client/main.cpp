#include "public.hpp"
#include "user.hpp"
#include "group.hpp"
#include "json.hpp"
#include <vector>
#include <thread>
#include <unordered_map>
#include <functional>
#include <ctime>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using json = nlohmann::json;

// 记录当前登录的用户信息
User currentUser;
// 记录当前用户的好友列表
vector<User> currentUser_FriendList;
// 记录当前登录用户的群组列表信息
vector<Group> currentUser_GroupList;
// 控制主菜单的布尔值
bool login_flag = false;
// 显示当前登录用户成功的基本信息
void showCurrentUser();

// 帮助函数
void help(int, string);
// 添加好友函数
void addFriend(int, string);
// 创建群组函数
void createGroup(int, string);
// 加入群组函数
void addGroup(int, string);
// 一对一聊天函数
void oneChat(int, string);
// 群聊函数
void groupChat(int, string);
// 注销函数
void logout(int, string);

// 客户端支持的命令集合
unordered_map<string, function<void(int, string)>> handler = {
    {"help", help},
    {"addFriend", addFriend},
    {"createGroup", createGroup},
    {"addGroup", addGroup},
    {"oneChat", oneChat},
    {"groupChat", groupChat},
    {"logout", logout}};

// 接收消息的线程
void readTaskHandler(int clientfd);
// 主界面
void mainMenu(int clientfd);

// 登录业务
void Login(int clientfd)
{
    string name;
    string password;
    cout << "用户名:";
    cin >> name;
    cout << "密码:";
    cin >> password;

    // 发送登录请求
    json js;
    js["msgid"] = LOGIN_MSG;
    js["name"] = name.c_str();
    js["password"] = password.c_str();
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送登录请求失败" << endl;
    }
    else
    {
        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (len == -1)
        {
            cerr << "接收登录响应失败" << endl;
        }
        else
        {
            json response = json::parse(buffer);
            if (response["errorid"].get<int>() != 0)
            {
                // 登录失败
                cerr << response["errortext"] << endl;
            }
            else
            {
                // 登陆成功
                // 记录当前用户的ID与用户名
                currentUser.setId(response["id"].get<int>());
                currentUser.setName(response["name"]);
                cout << "登录成功!" << endl;

                // 等待服务器端登录的详细数据
                while (true)
                {
                    memset(buffer, 0, sizeof(buffer));
                    len = recv(clientfd, buffer, 1024, 0);
                    if (len <= 0)
                    {
                        cerr << "接收用户信息失败" << endl;
                        return;
                    }

                    json info_response = json::parse(buffer);
                    if (info_response["msgid"].get<int>() == LOGIN_INFO_ACK)
                    {
                        // 记录当前用户的好友列表
                        if (info_response.contains("friends"))
                        {
                            // 初始化全局变量
                            currentUser_FriendList.clear();

                            vector<string> friends = info_response["friends"];
                            for (string &str : friends)
                            {
                                json js = json::parse(str);
                                User fri;
                                fri.setId(js["id"].get<int>());
                                fri.setName(js["name"]);
                                fri.setState(js["state"]);
                                currentUser_FriendList.push_back(fri);
                            }
                        }

                        // 记录当前用户的群组列表
                        if (info_response.contains("group"))
                        {
                            // 初始化全局变量
                            currentUser_GroupList.clear();

                            vector<string> group = info_response["group"];
                            for (string &g : group)
                            {
                                // 群组基本信息
                                json js_g = json::parse(g);
                                Group temp;
                                temp.setId(js_g["groupid"].get<int>());
                                temp.setName(js_g["name"]);
                                temp.setDesc(js_g["desc"]);

                                // 群组成员信息
                                vector<string> users_vec = js_g["users"];
                                for (string &user : users_vec)
                                {
                                    GroupUser mem;
                                    json user_js = json::parse(user);
                                    mem.setId(user_js["guserid"].get<int>());
                                    mem.setName(user_js["gusername"]);
                                    mem.setRole(user_js["guserrole"]);
                                    mem.setState(user_js["guserstate"]);

                                    temp.getUsers().push_back(mem);
                                }

                                currentUser_GroupList.push_back(temp);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUser();

                        // 显示当前用户的离线消息
                        if (info_response.contains("offlineMsg"))
                        {
                            // 离线信息 私聊与群聊的区分
                            // TODO...
                            cout << "============离线信息============" << endl;
                            unordered_map<string, vector<string>> offlineMsg = info_response["offlineMsg"].get<unordered_map<string, vector<string>>>();
                            for (auto &pair : offlineMsg)
                            {
                                for (auto &str : pair.second)
                                {
                                    cout << pair.first << "   " << str << endl;
                                }
                            }
                        }

                        break;
                    }
                }

                // 启动接收线程负责收取消息 (只启动一次,因为设置的全局变量无法关闭此线程)
                static int readTaskNum = 0;
                if (readTaskNum == 0)
                {
                    readTaskNum++;
                    thread t(readTaskHandler, clientfd);
                    t.detach();
                }

                // 进入聊天室主菜单界面
                login_flag = true;
                mainMenu(clientfd);
            }
        }
    }
}

// 注册业务
void Register(int clientfd)
{
    string name;
    string password;
    cout << "用户名:";
    cin >> name;
    cout << "密码:";
    cin >> password;

    json js;
    js["msgid"] = REG_MSG;
    js["name"] = name.c_str();
    js["password"] = password.c_str();
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送注册请求失败" << endl;
    }
    else
    {
        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (len == -1)
        {
            cerr << "接收注册响应失败" << endl;
        }
        else
        {
            json response = json::parse(buffer);
            if (response["errorid"].get<int>() != 0)
            {
                // 注册失败
                cerr << "注册失败!" << endl;
            }
            else
            {
                // 注册成功
                cout << "注册成功!" << endl;
            }
        }
    }
}

// 主线程用于发送消息
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "命令不合法!" << endl;
        exit(-1);
    }

    // 解析获取IP号与端口号
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建客户端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "Socket创建失败" << endl;
        exit(-1);
    }

    // 填写客户端需要连接的服务器信息
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 客户端与服务器端进行连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "连接服务器失败!" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        // 显示首界面菜单
        cout << "=========================" << endl;
        cout << "1.登录" << endl;
        cout << "2.注册" << endl;
        cout << "3.退出" << endl;
        cout << "=========================" << endl;
        cout << "您想执行什么指令?" << endl;
        int op = 0;
        cin >> op;
        // 根据op启动不同应用
        switch (op)
        {
        case 1:
            Login(clientfd);
            break;
        case 2:
            Register(clientfd);
            break;
        case 3:
            close(clientfd);
            exit(0);
            break;
        default:
            cerr << "命令非法" << endl;
            break;
        }
    }
}

void showCurrentUser()
{
    cout << "============用户信息============" << endl;
    cout << "用户名:" << currentUser.getName() << "    用户ID: " << currentUser.getId() << endl;
    cout << "============好友列表============" << endl;
    for (auto &fri : currentUser_FriendList)
    {
        cout << fri.getId() << "   " << fri.getName() << "   " << fri.getState() << endl;
    }
    cout << "============群组列表============" << endl;
    for (auto &group : currentUser_GroupList)
    {
        cout << group.getId() << "   " << group.getName() << endl;
        cout << group.getDesc() << endl;
    }
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len <= 0)
        {
            close(clientfd);
            exit(-1);
        }
        else
        {
            json js = json::parse(buffer);
            // 接收单发消息
            if (js["msgid"].get<int>() == ONE_CHAT_MSG)
            {
                cout << js["name"].get<string>() << "   私聊" << endl;
                cout << js["msg"].get<string>() << endl;
                continue;
            }
            // 接收群发消息
            if (js["msgid"].get<int>() == GROUP_CHAT_MSG)
            {
                cout << js["name"].get<string>() << "   群聊" << "(" << js["groupid"] << ")" << endl;
                cout << js["msg"].get<string>() << endl;
                continue;
            }
        }
    }
}

void mainMenu(int clientfd)
{
    cout << "输入help获取帮助文档" << endl;
    // 消除换行符
    cin.get();
    char buffer[1024] = {0};
    while (login_flag)
    {
        cin.getline(buffer, 1024);
        string command_src(buffer);
        string command;
        int idx = command_src.find(":");
        if (idx == -1)
        {
            command = command_src;
        }
        else
        {
            command = command_src.substr(0, idx);
        }
        auto it = handler.find(command);
        if (it == handler.end())
        {
            cout << "命令非法" << endl;
            continue;
        }
        // 执行命令原语对应的函数
        it->second(clientfd, command_src.substr(idx + 1));
    }
}

void help(int, string)
{
    cout << "help: 查询语法命令" << endl;
    cout << "addFriend: 添加好友命令 addFriend:friendid" << endl;
    cout << "createGroup: 创建群组命令 createGroup:groupname:groupdesc" << endl;
    cout << "addGroup: 加入群组命令 addGroup:groupid" << endl;
    cout << "oneChat: 一对一聊天命令 oneChat:friendid:message" << endl;
    cout << "groupChat: 群聊命令 groupChat:groupid:message" << endl;
    cout << "logout: 注销命令" << endl;
}

void addFriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送添加好友请求失败" << endl;
    }
}

void createGroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "命令非法" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送创建群组请求失败" << endl;
    }
}

void addGroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送添加群组请求失败" << endl;
    }
}

void oneChat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "命令非法" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["name"] = currentUser.getName();
    js["from"] = currentUser.getId();
    js["to"] = friendid;
    js["msg"] = message;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送一对一聊天请求失败" << endl;
    }
}

void groupChat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "命令非法" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["name"] = currentUser.getName();
    js["id"] = currentUser.getId();
    js["groupid"] = groupid;
    js["msg"] = message;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "发送群聊请求失败" << endl;
    }
}

void logout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "请求失败!" << endl;
    }

    login_flag = false;
}