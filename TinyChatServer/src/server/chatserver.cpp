// stl
#include <string>
#include <functional>
// local files
#include "json.hpp"
#include "chatserver.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
        const InetAddress& listenAddr,
        const string& nameArg)
        :_server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    /*绑定器通俗来讲就是：一个函数需要两个参数，我们只想暴露一个参数，另一个由我们指定
    也就是两个参数的函数编程一个参数的函数，这时候就可以用函数绑定器
    */
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
            
}

ChatServer::~ChatServer(){//@todo this is a test
    
}

// 启动服务
void ChatServer::start(){
    _server.start();
}

// 上报链接相关的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn){//@remind this is a
    // 客户端断开链接
    if(!conn->connected()){
        ChatService::Instance()->ClientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn,
               Buffer* buffer,
               Timestamp time){
    string buf = buffer->retrieveAllAsString();
    // 数据反序列化
    json js = json::parse(buf);
    // 通过[message id]获取业务handler, connection , js, time
    auto msg_handler = ChatService::Instance()->GetHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msg_handler(conn, js, time);
}
