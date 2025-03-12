#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// 用户表的映射类
class User
{
private:
    int id;
    string name;
    string password;
    string state;

public:
    User(int id = -1, string n = "", string p = "", string s = "offline")
    {
        this->id = id;
        this->name = n;
        this->password = p;
        this->state = s;
    }
    void setId(int id) { this->id = id; }
    void setName(string n) { this->name = n; }
    void setPassword(string p) { this->password = p; }
    void setState(string s) { this->state = s; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPassword() { return this->password; }
    string getState() { return this->state; }
};

#endif