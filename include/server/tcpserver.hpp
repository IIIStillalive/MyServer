#pragma once

#include <functional>
#include <memory>
#include <netinet/in.h>   // struct sockaddr_in
#include <vector>

#include "acceptor.hpp"
#include "tcp_connnection.hpp"

namespace server{

class EventLoop;  // 前向声明；仅存指针，无需完整类型

// TcpServer：面向使用者的"服务器"门面。
//   把 main 里散开的服务器组装收敛进这个类：
//     - 持有监听的 Socket（经 Acceptor）与全部活动连接
//     - 收到新连接 → 建 TcpConnection 接管
//     - 连接关闭 → 回调本类，从容器移除并释放（unique_ptr 析构 → sock_ close fd）
//   对外只暴露 start()：启动监听并挂入事件循环。
class TcpServer{

public:
    TcpServer(EventLoop* loop, const struct sockaddr_in& addr);
    ~TcpServer() = default;

    bool start();  // 建 Acceptor + 注册新连接回调 + 挂上 epoll；失败返回 false

private:
    void onNewConnection(int cfd);  // Acceptor 回调：接管一个已 accept 的连接

    EventLoop* loop_;
    Acceptor acceptor_;                                // 监听方（持有 listen fd）
    std::vector<std::unique_ptr<TcpConnection>> conns_;  // 持有活动连接，活过 runloop()
};

}