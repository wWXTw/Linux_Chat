#ifndef OFFLINE_MESSAGE_MODEL_H
#define OFFLINE_MESSAGE_MODEL_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <mysql.h>

using namespace std;

class OfflineMessageModel
{
public:
    // 存储用户的离线消息
    void insert(int userid, string msg, string name);

    // 删除用户的离线消息
    void remove(int userid);

    // 加载用户的离线消息
    unordered_map<string, vector<string>> query(int userid);
};

#endif