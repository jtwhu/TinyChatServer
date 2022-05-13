#pragma once
// stl
#include <mutex>
#include <functional>
#include <unordered_map>
// third party
#include <muduo/net/TcpConnection.h>
// local files
#include "json.hpp"
#include "redis.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "offlinemessagemodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

// 表示处理消息的时间回调方法类型
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;

// 使用单例模式设计对象
// 聊天服务器业务类
class  ChatService
{
private:
    // 数据操作类对象
    UserModel _userModel;
    // redis操作对象
    Redis _redis;
    // 消息处理器表，存储消息ID及其对应的处理操作
    unordered_map<int, MsgHandler> _msgHandlerMap;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
    // 离线消息表
    OfflineMsgModel _offlineMsgModel;
    // 好友列表
    FriendModel _friendModel;
    // 群组列表
    GroupModel _groupModel;
    // 单例模式，构造函数私有化
    ChatService(/* args */);

public:
    // 获取单例对象的接口函数
    static ChatService* Instance();
    // 处理登录业务
    void Login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void Reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void OneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void AddFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void CreateGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void AddGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void GroupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void LoginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void ClientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void Reset();
    // 获取消息对应的处理器
    MsgHandler GetHandler(int msgid);
    // 获取redis消息队列中订阅的消息(channel对应于userid)
    void HandleRedisSubscribeMessage(int channel, string msg);
    ~ ChatService();
};
