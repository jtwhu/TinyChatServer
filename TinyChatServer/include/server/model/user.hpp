#pragma once 
// stl
 #include <string>

// 匹配user表的ORM类,映射表的字段
 class User
 {
 protected:
     /* data */
     int _id;
     std::string _name;
     std::string _password;
     std::string _state;
 public:
    void SetId(int id){this->_id = id;}
    void SetName(std::string name){this->_name = name;}
    void SetPwd(std::string pwd){this->_password = pwd;}
    void SetStat(std::string state){this->_state = state;}
    
    int GetId(){return this->_id;}
    std::string GetName(){return this->_name;}
    std::string GetPwd(){return this->_password;}
    std::string GetStat(){return this->_state;}
    
    User(int id = -1, std::string name = "", std::string pwd = "", std::string state = "offline")
    :_id(id), _name(name), _password(pwd), _state(state){}
    ~User(){}
 };