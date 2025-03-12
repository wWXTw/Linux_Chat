#ifndef USER_MODEL_H
#define USER_MODEL_H

#include "user.hpp"

// 用户表的操作类
class UserModel
{
public:
    // User表的增加方法
    bool insertUser(User &user);

    // User表的查询方法(通过ID号)
    User queryUserId(int id);

    // User表的查询方法(通过用户名)
    User queryUserName(string name);


    // User表的状态更新方法
    void updateState(User user);

    // User表的状态重置方法
    void resetState();
};

#endif