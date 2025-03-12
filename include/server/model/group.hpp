#ifndef GROUP_H
#define GROUP_H

#include <iostream>
#include <vector>
#include "groupuser.hpp"

using namespace std;

class Group
{
private:
    int groupid;
    string name;
    string desc;
    vector<GroupUser> users;

public:
    Group(int gid = -1, string n = "", string d = "")
    {
        groupid = gid;
        name = n;
        desc = d;
    }

    void setId(int id) { groupid = id; }
    void setName(string n) { name = n; }
    void setDesc(string d) { desc = d; }

    int getId() { return groupid; }
    string getName() { return name; }
    string getDesc() { return desc; }
    vector<GroupUser> &getUsers() { return users; }
};

#endif