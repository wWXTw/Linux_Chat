#ifndef PUBLIC_H
#define PUBLIC_H

// 客户端与服务器端的公共文件
enum MsgType
{
    // 登陆消息
    LOGIN_MSG = 1,
    // 登录响应消息
    LOGIN_MSG_ACK,
    // 登录具体信息消息
    LOGIN_INFO_ACK,
    // 登出消息
    LOGOUT_MSG,
    // 注册消息
    REG_MSG,
    // 注册响应信息
    REG_MSG_ACK,
    // 一对一聊天
    ONE_CHAT_MSG,
    // 添加好友消息
    ADD_FRIEND_MSG,
    // 创建群组
    CREATE_GROUP_MSG,
    // 加入群组
    ADD_GROUP_MSG,
    // 群聊天
    GROUP_CHAT_MSG,
};

#endif