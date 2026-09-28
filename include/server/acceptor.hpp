#pragma once

#include <functional>
#include <netinet/in.h>   // struct sockaddr_in

#include "channel.hpp"
#include "socket.hpp"

namespace server{

class EventLoop;  // 前向声明；.cpp 里才需要完整类型

// 监听方：负责"一个被监听的端口"的全部事务。
//   资源：监听 fd（由 Socket 持有、RAII 关闭）
//   分发：监听 fd 上的可读事件 → accept 循环（ET 读空队列）
//   上层：通过 setNewConnectionCallback 拿到已 accept 的 cfd（Socket 所有权随之转移给上层）
// 注意：监听 fd 不是 TcpConnection——它只 accept，不读业务数据。
class Acceptor{

public:
    Acceptor(EventLoop* loop, const struct sockaddr_in& addr);
    ~Acceptor() = default;

    // 注册 accept 回调并把监听 fd 挂上 epoll；失败返回 false
    bool listen();

    // 每 accept 到一个新连接，把 cfd 交给上层（例外：该 fd 的资源所有权由上层接管）
    void setNewConnectionCallback(std::function<void(int)> cb);

private:
    void onAccept();  // ET 语义：accept 到 EAGAIN，逐个回调

    Socket sock_;             // 监听 fd 的资源所有者（唯一 close 方）
    Channel ch_;              // 监听 fd 的事件分发器（不拥有 fd）
    EventLoop* loop_;
    std::function<void(int)> newConnectionCallback_;
};

}