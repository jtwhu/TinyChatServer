#pragma once
// local files
#include "user.hpp"

// usr表的数据操作类
class UserModel
{
public:
    // user表的增加方法
    bool Insert(User &user);
    // 根据用户号码查询用户信息
    User Query(int id);
    // 更新用户的状态信息
    bool UpdateState(User user);
};

