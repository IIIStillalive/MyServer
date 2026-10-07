#pragma once

#include <functional>
#include <memory>

#include "buffer.hpp" 

#include "channel.hpp"
#include "socket.hpp"
namespace server{

class Buffer;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>{  //管理一整个连接

public:

    TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection() = default;

    bool listenconnection();
    bool connection();

    int fd();
    void loop();  // 工作循环

    // 预留回调接口：连接关闭时通知所有者(main)把我释放；所有权属于外部
    void setCloseCallback(std::function<void(TcpConnection*)> cb);
    void send(const void* data, size_t n);   // public：业务侧发送入口
    using MessageCallback = std::function<void(TcpConnection*, Buffer*)>;
    void setMessageCallback(MessageCallback cb);   // 业务层挂钩：onRead 读到的字节交给它解析

private:
    void onWrite();                          // 可写事件回调
    void writeNow();                         // 把 outputBuffer_ 尽力发一截
    Buffer inputBuffer_;                     // 输入缓冲
    Buffer outputBuffer_;                    // 输出缓冲（待发累积）

    void handleClose();
    void handleError();
    void onRead();
    void onError();
    int set_nonblocking();
    Socket sock_;  //持有socket资源
    Channel ch_;  //持有channel资源，负责分发
    EventLoop* loop_;  //持有一个loop指针, 负责epoll事务

    std::function<void(TcpConnection*)> closeCallback_;  // 点对点的释放通知
    MessageCallback messageCallback_;                    // 连接层读到字节后的业务分发
    bool closed_ = false;  // 防重复关闭

};

}