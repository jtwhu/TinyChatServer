// local files
#include "db.h"
#include "offlinemessagemodel.hpp"


// 存储用户的离线消息
bool OfflineMsgModel::Insert(int userid, std::string msg){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values('%d', '%s')", userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;// 存储失败
}

// 删除用户的离线消息
bool OfflineMsgModel::Remove(int userid){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
        return true;
    }
    return false;
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsgModel::Query(int userid){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    MySQL mysql;
    std::vector<std::string> vec;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){   
            MYSQL_ROW row;
            // 把userid用户的所有（多行）离线消息放入到vec中
            while((row=mysql_fetch_row(res)) != nullptr){
                vec.push_back(row[0]);
            }

            // 释放资源（指针的内部是申请了内存的）
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;// 或者直接返回空
}

