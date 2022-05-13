#include "usermodel.hpp"
#include "db.h"

// user表的增加方法
 bool UserModel::Insert(User &user){
     // 1.组装sql语句
     char sql[1024] = {0};
     sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')", 
      user.GetName().c_str(), user.GetPwd().c_str(), user.GetStat().c_str());

      MySQL mysql;
      if(mysql.connect()){
          if(mysql.update(sql)){
              // 获取插入成功的用户数据生成的id
              user.SetId(mysql_insert_id(mysql.getConnection()));
              return true;
          }
      }
      return false;// 注册失败
 }

// 根据用户号码查询用户信息
User UserModel::Query(int id){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d",id);

     MySQL mysql;
     if(mysql.connect()){
         MYSQL_RES* res = mysql.query(sql);// 此处申请的内存需要手动释放

         if(res != nullptr){
             MYSQL_ROW row = mysql_fetch_row(res);
             if(row != nullptr){
                 User user;
                 // row分别对应四个字段
                 user.SetId(atoi(row[0]));// 转换成为整数
                 user.SetName(row[1]);
                 user.SetPwd(row[2]);
                 user.SetStat(row[3]);
                 mysql_free_result(res);// 释放内存，防止内存泄漏
                 return user;
             }
         }
     }
     return false;// 注册失败
}

// 更新用户的状态信息
bool UserModel::UpdateState(User user){
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",user.GetStat().c_str(), user.GetId());

     MySQL mysql;
     if(mysql.connect()){
         // 出作用域mysql析构，自动释放连接资源
         if(mysql.update(sql)){
             return true;
         }
     }
     return false;// 更新状态失败
}