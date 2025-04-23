### 基于Linux系统下的集群聊天器  
1. 利用 **Json** 序列化与反序列化进行服务器与客户端的信息传递 ( json for modern cpp )  
2. 利用 **muduo库** 来实现服务器端的多线程响应与简化网络编程
3. 利用 **nginx** 设置负载均衡器,为服务器集群提供负载均衡
4. 利用 **kafka** 作为中间件,实现跨平台的服务器通讯

2025/4/6 - version 2.0  
将中间件由Redis的publish/subscribe改写为kafka

2025/4/23 - version 2.1
修改客户端与服务端的TCP发送接收过程,防止粘包/拆包问题

