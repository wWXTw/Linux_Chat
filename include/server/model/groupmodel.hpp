#ifndef GROUP_MODEL_H
#define GROUP_MODEL_H

#include "group.hpp"
#include <string>
#include <vector>

using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    void addGroup(int userid, int groupid, string role);

    // 查询用户所在群组信息
    vector<Group> queryGroup(int userid);

    // 查询某一群组内其他成员的ID (群发消息)
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif