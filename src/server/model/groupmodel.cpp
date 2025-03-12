#include "groupmodel.hpp"
#include "mysql.h"

bool GroupModel::createGroup(Group &group)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL db;
    if (db.connect())
    {
        if (db.update(sql))
        {
            // 设置该群组类变量的id
            group.setId(mysql_insert_id(db.getConnection()));
            return true;
        }
    }
    return false;
}

void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values(%d,%d,'%s')",
            groupid, userid, role.c_str());
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}

vector<Group> GroupModel::queryGroup(int userid)
{
    vector<Group> result;
    // 组装SQL语句 双表联合查询
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on b.groupid = a.id where b.userid=%d",
            userid);
    MySQL db;
    if (db.connect())
    {
        MYSQL_RES *res = db.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                result.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &g : result)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a inner join GroupUser b on a.id=b.userid where b.groupid=%d",
                g.getId());
        MySQL db;
        if (db.connect())
        {
            MYSQL_RES *res = db.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    g.getUsers().push_back(user);
                }
                mysql_free_result(res);
            }
        }
    }

    return result;
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    vector<int> result;
    // 组装SQL语句 双表联合查询
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid=%d and userid!=%d",
            groupid, userid);
    MySQL db;
    if (db.connect())
    {
        MYSQL_RES *res = db.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                result.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }

    return result;
}