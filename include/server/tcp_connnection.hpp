#pragma once

#include <functional>

#include "channel.hpp"
#include "socket.hpp"
namespace server{

class TcpConnection{  //管理一整个连接

public:

    TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection() = default;

    bool listenconnection();
    bool connection();

    int fd();
    void loop();  // 工作循环

    // 预留回调接口：连接关闭时通知所有者(main)把我释放；所有权属于外部
    void setCloseCallback(std::function<void(TcpConnection*)> cb);

private:
    void handleClose();
    void handleError();
    void onRead();
    void onError();
    int set_nonblocking();
    Socket sock_;  //持有socket资源
    Channel ch_;  //持有channel资源，负责分发
    EventLoop* loop_;  //持有一个loop指针, 负责epoll事务

    std::function<void(TcpConnection*)> closeCallback_;  // 点对点的释放通知
    bool closed_ = false;  // 防重复关闭

};

}