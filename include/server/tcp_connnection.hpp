#pragma once

#include <fcntl.h>

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
private:
    int set_nonblocking();
    Socket sock_;  //持有socket资源
    Channel ch_;  //持有channel资源，负责分发
    EventLoop* loop_;  //持有一个loop指针, 负责epoll事务

};

TcpConnection::TcpConnection(int fd, EventLoop* loop): sock_(fd), ch_(fd, loop), loop_(loop){}
int TcpConnection::set_nonblocking(){
    int fd = sock_.fd();
    int flags = ::fcntl(fd, F_GETFL, 0);
    if(flags == -1) return -1;
    if(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}
bool TcpConnection::listenconnection(){
    if(set_nonblocking() < 0) return false;  //设置为非堵塞
    if(!(this->loop_->updatachannel(&ch_))) return false;  //上树
    return true;
}

}

