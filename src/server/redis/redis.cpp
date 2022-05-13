// stl
#include <thread>
#include <iostream>
// local files
#include "redis.hpp"

using namespace std;

Redis::Redis()
    :_publishContext(nullptr), _subscribeContext(nullptr){
}

Redis::~Redis(){
    if(_publishContext != nullptr){
        redisFree(_publishContext);
    }
    if(_subscribeContext != nullptr){
        redisFree(_subscribeContext);
    }
}

// 连接redis服务器
bool Redis::Connect(){
    // 负责发布消息的上下文连接
    _publishContext = redisConnect("127.0.0.1", 6379);
    if(nullptr == _publishContext){
        cerr << "connect redis failed" << endl;
        return false;
    }

    // 负责订阅消息的上下文连接
    _subscribeContext = redisConnect("127.0.0.1", 6379);
    if(nullptr == _subscribeContext){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&](){
        ObserverChannelMessage();
    });// 这里线程的参数直接使用的lambda表达式，匿名函数
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::Publish(int channel, std::string message){
    redisReply *reply = (redisReply*)redisCommand(_publishContext, "PUBLISH %d %s", channel, message.c_str());
    if(nullptr == reply){
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::Subscribe(int channel){
    // SUBSCRICE命令本身会造成线程阻塞等待通道里发生消息，这里只做订阅消息，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server的响应消息，否则会和notifyMsg线程抢占响应资源？？？？
    if(REDIS_ERR == redisAppendCommand(this->_subscribeContext, "SUBSCRIBE %d", channel)){\
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBuffer可以循环发送缓冲区，直到缓冲区数据发送完毕（done设置为1）
    int done = 0;
    while(!done){
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done)){
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetreply
    return true;
}

// 向redis指定的通道unscribe取消订阅消息
bool Redis::Unsubscribe(int channel){
    if(REDIS_ERR == redisAppendCommand(this->_subscribeContext, "UNSUBSCRIBE %d", channel)){
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被设置为1）
    int done = 0;
    while(!done){// 这一块的代码逻辑有点迷,直接if判断一下不就行了？
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done)){
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::ObserverChannelMessage(){
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribeContext, (void **)& reply)){
        // 订阅到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            // 给业务层上报通道上发生的消息
            _notifyMessageHandler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<<<<<" << endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::InitNotifyHandler(std::function<void(int, std::string)> fn){
    this->_notifyMessageHandler = fn;
}