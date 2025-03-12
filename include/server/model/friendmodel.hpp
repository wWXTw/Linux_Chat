#ifndef FRIEND_MODEL_H
#define FRIEND_MODEL_H

#include <iostream>
#include <string>
#include <vector>
#include "user.hpp"
#include "mysql.h"
using namespace std;

class FriendModel
{
public:
    // 添加好友关系
    void insert(int userid, int friendid);

    // 返回好友列表
    vector<User> query(int userid);
};

#endif