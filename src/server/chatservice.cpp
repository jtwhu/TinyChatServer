// stl
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
// third party
#include "group.hpp"
#include <muduo/base/Logging.h>
// local files
#include "user.hpp"
#include "redis.hpp"
#include "public.hpp"
#include "usermodel.hpp"
#include "chatservice.hpp"
#include "friendmodel.hpp"

using namespace muduo;

ChatService* ChatService::Instance(){
    // 线程安全的单例模式回头去复习
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService(/* args */){
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::Login, this, _1, _2, _3)});// 绑定消息与对应的事件处理器
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::LoginOut, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::Reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::OneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::AddFriend, this, _1, _2, _3)});
    
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::CreateGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::AddGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::GroupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if(_redis.Connect()){
        // 设置上报消息的回调
        _redis.InitNotifyHandler(std::bind(&ChatService::HandleRedisSubscribeMessage, this, _1, _2));
    }
}

ChatService::~ChatService(/* args */){

}

// 处理登录业务 id pwd pwd
 void ChatService::Login(const TcpConnectionPtr& conn, json& js, Timestamp time){
     int id = js["id"].get<int>();
     string pwd = js["password"];

     User user = _userModel.Query(id);
     if(user.GetId() == id && user.GetPwd() == pwd){
         if(user.GetStat() == "online"){
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，请重新输入账号";
            conn->send(response.dump());

         }else{// 登录成功
            // 记录用户连接信息，小技巧：使用局部作用域让锁自动释放
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            user.SetStat("online");//更新用户状态信息

            _userModel.UpdateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.GetId();
            response["name"] = user.GetName();
            // 查询用户是否存在离线消息
            std::vector<std::string> vec = _offlineMsgModel.Query(user.GetId());
            if(!vec.empty()){
                response["offlinemsg"] = vec;// json库可直接序列化STL库的容器
                // 读取用户的离线消息之后，将该用户的离线消息表清空
                _offlineMsgModel.Remove(user.GetId());
            }
            // 查询该用户的好友信息并返回
            std::vector<User> friendVec = _friendModel.Query(id);
            if(!friendVec.empty()){
                std::vector<std::string> friendInfoVec;
                for(User &myfriend : friendVec){
                    json js;
                    js["id"] = myfriend.GetId();
                    js["name"] = myfriend.GetName();
                    js["state"] = myfriend.GetStat();
                    friendInfoVec.push_back(js.dump());
                }
                // 返回解析后的用户信息
                response["friends"] = friendInfoVec;
            }
            // 查询用户的群组信息
            std::vector<Group> groupVec = _groupModel.QueryGroups(id);
            if(!groupVec.empty()){
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                // 群组基本信息
                std::vector<std::string> groupInfoVec;
                for(Group &group : groupVec){
                    json grpjson;
                    grpjson["id"] = group.GetID();
                    grpjson["groupname"] = group.GetName();
                    grpjson["groupdesc"] = group.GetDesc();
                    std::vector<std::string> groupUserInfoVec;
                    // 群组用户信息
                    for(GroupUser &user : group.GetUsers()){
                        json uerjson;
                        js["id"] = user.GetId();
                        js["name"] = user.GetName();
                        js["state"] = user.GetStat();
                        js["role"] = user.GetRole();
                        groupUserInfoVec.push_back(js.dump());
                    }
                    grpjson["users"] = groupUserInfoVec;
                    groupInfoVec.push_back(grpjson.dump());
                }
            }
            conn->send(response.dump());
         }
        
     }else{
        // 该用户不存在，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
     }
 }

// 处理注册业务
void ChatService::Reg(const TcpConnectionPtr& conn, json& js, Timestamp time){
     string name = js["name"];
     string pwd  = js["password"];

     // 生成新用户
     User user;
     user.SetName(name);
     user.SetPwd(pwd);

     // 插入新用户
     bool state = _userModel.Insert(user);

     if(state){
         // 注册成功
         json response;
         response["msgid"] = REG_MSG_ACK;
         response["errno"] = 0;// errno为0表示注册成功
         response["id"] = user.GetId();
         conn->send(response.dump());
     }else{
         // 注册失败
         json response;
         response["msgid"] = REG_MSG_ACK;
         response["errno"] = 1;// errno为1表示注册失败
         conn->send(response.dump());
     }
}

// 获取消息对应的处理器
MsgHandler ChatService::GetHandler(int msgid){
    // 记录错误日志，msgid没有对应的事件处理回调
    auto iter = _msgHandlerMap.find(msgid);
    if(iter == _msgHandlerMap.end()){
        // 返回一个默认处理器
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time){
            LOG_ERROR << "msgid" << msgid << "can not find a handler!";
        };

    }
    return this->_msgHandlerMap[msgid];
}

// 处理客户端异常退出
void ChatService::ClientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex);

        for(auto iter = _userConnMap.begin(); iter != _userConnMap.end(); ++iter){
            if(iter->second == conn){
                
                user.SetId(iter->first); // 获取当前连接的用户信息
                _userConnMap.erase(iter);// 从map表中删除连接信息
                break;
            }
        }
    }
    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.Unsubscribe(user.GetId());

    // 删除用户状态信息
    if(user.GetId() != -1){
        user.SetStat("offline");
        _userModel.UpdateState(user);
    }
}

// 服务器异常，业务重置方法
void ChatService::Reset(){
    // online用户设置为offline
    
}

// 一对一聊天业务
void ChatService::OneChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int toid = js["toid"].get<int>();// 转换为整型
    // 访问连接信息表，必须保证其线程安全性
    {
        lock_guard<mutex> lock(_connMutex);// 对连接信息表上锁，出作用域自动解锁
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            // toid在线，转发消息
            it->second->send(js.dump());// 转发消息到对方客户端
            return;
        }
    }
    // 以上查询为同一台服务器上的查询，对方不在线，接下来查询数据库中对方是否在线。若在线，使用服务器中间件发送消息
    User user = _userModel.Query(toid);
    if(user.GetStat() == "online"){
        _redis.Publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.Insert(toid, js.dump());
}

// 添加好友业务
void ChatService::AddFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.Insert(userid, friendid);// 用户id和friendid
}

// 创建群组业务
void ChatService::CreateGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.CreateGroup(group)){
        // 存储创建人信息
        _groupModel.AddGroup(userid, group.GetID(), "creator");
    }
}

// 加入群组业务
void ChatService::AddGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.AddGroup(userid, groupid, "normal");// 后续加入的群成员的角色为normal
}

// 群组聊天业务 (群聊就是从群组对象中取出当前群组的所有用户id，遍历发送消息)
void ChatService::GroupChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = _groupModel.QueryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);// 访问_userConnMap,需要保证线程安全
    for(int id : useridVec){
        auto iter = _userConnMap.find(id);
        if(iter != _userConnMap.end()){
            iter->second->send(js.dump());// 在线并且是群成员直接转发群消息
        }else{
            // 查询toid是否在线
            User user = _userModel.Query(id);
            if(user.GetStat() == "offline"){
                // 存储离线消息
                _offlineMsgModel.Insert(id, js.dump());
            }else{
                // 说明对方注册在另外的服务器中，跨服务器进行通信
                _redis.Publish(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::LoginOut(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto iter = _userConnMap.find(userid);
        if(iter != _userConnMap.end()){
            _userConnMap.erase(iter);// 断开连接，删除连接信息
        }
    }
    // 更细用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.UpdateState(user);// 注意一下这里如何更改用户的装态信息的：使用原userid创建state=offline的新user
}

// 从redis消息队列中获取订阅的消息(channel对应于userid)
void ChatService::HandleRedisSubscribeMessage(int channel, string msg){
    lock_guard<mutex> lock(_connMutex);
    auto iter = _userConnMap.find(channel);
    if(iter != _userConnMap.end()){
        iter->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.Insert(channel, msg);
}