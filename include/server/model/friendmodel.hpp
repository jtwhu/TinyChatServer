#pragma once
// stl
#include <vector>
// local files
#include "user.hpp"

class FriendModel{
public:
    // 添加好友关系
    bool Insert(int userid, int friendid);
    // 返回用户好友列表
    std::vector<User> Query(int userid);
};