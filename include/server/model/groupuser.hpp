#ifndef GROUP_USER_H
#define GROUP_USER_H

#include "user.hpp"

class GroupUser : public User
{
private:
    // 群组中的角色
    string role;

public:
    void setRole(string r) { role = r; }

    string getRole() { return role; }
};

#endif