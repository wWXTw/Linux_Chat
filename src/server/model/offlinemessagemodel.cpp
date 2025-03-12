#include "offlinemessagemodel.hpp"

void OfflineMessageModel::insert(int userid, string msg, string name)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values(%d,'%s','%s')",
            userid, msg.c_str(), name.c_str());
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}

void OfflineMessageModel::remove(int userid)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid = %d",
            userid);
    MySQL db;
    if (db.connect())
    {
        db.update(sql);
    }
}

unordered_map<string, vector<string>> OfflineMessageModel::query(int userid)
{
    unordered_map<string, vector<string>> result;
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select message,name from OfflineMessage where userid=%d",
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
                result[row[1]].push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return result;
}