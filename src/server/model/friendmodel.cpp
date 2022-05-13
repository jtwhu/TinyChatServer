// local files
#include "db.h"
#include "user.hpp"
#include "friendmodel.hpp"

// 添加好友关系
    // 1.组装sql语句
bool FriendModel::Insert(int userid, int friendid){
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d', '%d')", 
    userid, friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
        return true;
    }
    return false;// 注册失败
}

// 返回用户好友列表
std::vector<User> FriendModel::Query(int userid){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d"
    , userid);// 多表联合查询

    MySQL mysql;
    std::vector<User> vec;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);// 此处申请的内存需要手动释放

        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                User user;
                user.SetId(atoi(row[0]));
                user.SetName(row[1]);
                user.SetStat(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;// 注册失败
}