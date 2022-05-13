# TinyChatServer
 > 💡 **A tiny clustered chatserver based on  the Muduo  network library**

### 项目概述
> 
### 技术要点
- 使用muduo网络库作为项目网络核心模块，提供给高并发网络IO服务，解耦网络和业务模块代码
- 使用json序列反序列化消息作为私有通信协议
- 使用MySQL关系型数据库为项目数据提供落地存储
- 基于redis的发布订阅功能实现跨服务器的消息通信
- 配置Nginx基于tcp的负载均衡，实现聊天服务器的集群功能，提高后端服务的并发能力
### 业务流程
### 后续改进
- 使用数据库连接池提高数据库存取性能
### 参考资料