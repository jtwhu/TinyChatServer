#pragma once
// stl
#include <string>
// local files
#include "user.hpp"

// 群组用户，多了一个role角色信息，从user类直接继承，复用user的其他信息
class GroupUser : public User 
{
private:
    /* data */
    std::string _role;
public:
    GroupUser(/* args */){}
    ~GroupUser(){}
    void SetRole(std::string role){this->_role = role;}
    std::string GetRole(){return this->_role;}
};
