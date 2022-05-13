#pragma once
#include <hiredis/hiredis.h>
#include <string>
#include <functional>

/*
redis作为集群服务器通信的基于发布-订阅消息队列时，会遇到两个难搞的bug问题，参考资料如下：
https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611
*/
class Redis
{
public:
    Redis();
    ~Redis();
    // 连接redis服务器
    bool Connect();
    // 向redis指定的通道channel发布消息
    bool Publish(int channel, std::string message);
    // 向redis指定的通道subscribe订阅消息
    bool Subscribe(int channel);
    // 向redis指定的通道unscribe取消订阅消息
    bool Unsubscribe(int channel);
    // 在独立线程中接收订阅通道中的消息
    void ObserverChannelMessage();
    // 初始化向业务层上报通道消息的回调对象
    void InitNotifyHandler(std::function<void(int, std::string)> fn);
private:
    // hiredis同步上下文对象，负责publis消息
    redisContext *_publishContext;
    // hiredis同步上下文对象，负责subscribe消息
    redisContext *_subscribeContext;
    // 回调操作，受到订阅的消息之后，给service层进行上报
    std::function<void(int, std::string)> _notifyMessageHandler;
};

