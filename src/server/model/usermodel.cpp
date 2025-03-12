#include "usermodel.hpp"
#include "mysql.h"
#include <iostream>

using namespace std;

bool UserModel::insertUser(User &user)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    MySQL db;
    if (db.connect())
    {
        if (db.update(sql))
        {
            // 设置该用户类变量的id
            user.setId(mysql_insert_id(db.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::queryUserId(int id)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id=%d",
            id);
    MySQL db;
    User u;
    if (db.connect())
    {
        MYSQL_RES *res = db.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            u.setId(atoi(row[0]));
            u.setName(row[1]);
            u.setPassword(row[2]);
            u.setState(row[3]);
            // 释放资源预防内存泄漏
            mysql_free_result(res);
        }
    }
    return u;
}

User UserModel::queryUserName(string name)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where name='%s'",
            name.c_str());
    MySQL db;
    User u;
    if (db.connect())
    {
        MYSQL_RES *res = db.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            u.setId(atoi(row[0]));
            u.setName(row[1]);
            u.setPassword(row[2]);
            u.setState(row[3]);
            // 释放资源预防内存泄漏
            mysql_free_result(res);
        }
    }
    return u;
}

void UserModel::updateState(User user)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d ",
            user.getState().c_str(), user.getId());
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}

void UserModel::resetState()
{
    // 组装SQL语句
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}