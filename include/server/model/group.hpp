#pragma once
// stl
#include <string>
#include <vector>
// local files
#include "groupuser.hpp"

class Group
{
private:
    /* data */
    int _id;
    std::string _name;
    std::string _desc;
    std::vector<GroupUser> _users;
public:
    Group(int id = -1, std::string name = "", std::string desc = "")
    :_id(id),_name(name),_desc(desc){}

    ~Group(){}

    void SetId(int id){this->_id = id;}
    void SetName(std::string name){this->_name = name;}
    void SetDesc(std::string desc){this->_desc = desc;}
    
    int GetID(){return this->_id;}
    std::string GetName(){return this->_name;}
    std::string GetDesc(){return this->_desc;}
    std::vector<GroupUser>& GetUsers(){return this->_users;}
};
