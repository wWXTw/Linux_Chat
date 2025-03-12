#include "friendmodel.hpp"

void FriendModel::insert(int userid, int friendid)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values(%d,%d)",
            userid, friendid);
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}

vector<User> FriendModel::query(int userid)
{
    vector<User> result;
    // 组装SQL语句 双表联合查询
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from User a inner join Friend b on b.friendid = a.id where b.userid=%d",
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
                User fri;
                fri.setId(atoi(row[0]));
                fri.setName(row[1]);
                fri.setState(row[2]);
                result.push_back(fri);
            }
            mysql_free_result(res);
        }
    }
    return result;
}