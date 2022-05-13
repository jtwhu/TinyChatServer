// local files
#include "db.h"
#include "groupmodel.hpp"

// 创建群组
bool GroupModel::CreateGroup(Group &group){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')"
    ,group.GetName().c_str(), group.GetDesc().c_str());

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.SetId(mysql_insert_id(mysql.getConnection()));// 创建新群组之后插入id
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::AddGroup(int userid, int groupid, std::string role){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')",
    groupid, userid, role.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

// 查询用户所在的群组消息
std::vector<Group> GroupModel::QueryGroups(int userid){
    /*
    1.首先根据userid在groupuser表中查询出该用户所属的群组信息
    2.再根据群组信息，查询属于该群组的所有用户的userid,将goroupuer表和user表进行多表联合查询，查出用户的详细信息
    */
   char sql[1024] = {0};
   sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
   groupuser b on a.id=b.groupid where b.userid=%d", userid);
   
   vector<Group> groupVec;

   MySQL mysql;
   if(mysql.connect()){
       MYSQL_RES *res = mysql.query(sql);
       if(res != nullptr){
           MYSQL_ROW row;
           // 查出userid的所有群组信息
           while((row = mysql_fetch_row(res)) != nullptr){
               Group group;
               group.SetId(atoi(row[0]));
               group.SetName(row[1]);
               group.SetDesc(row[2]);
               
               groupVec.push_back(group);
           }
           mysql_free_result(res);
       }
   }
   return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员群发消息
std::vector<int> GroupModel::QueryGroupUsers(int userid, int groupid){
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d",
    groupid, userid);

    std::vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}