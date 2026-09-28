#include "acceptor.hpp"

#include <cerrno>
#include <cstring>       // memset
#include <fcntl.h>       // fcntl/O_NONBLOCK
#include <sys/socket.h>  // socket/bind/listen/setsockopt
#include <unistd.h>

#include "server/errno.hpp"
#include "server/log.hpp"

// 需要 EventLoop 完整类型：listen() 里 ch_.enablereading() → update() → loop_->updatachannel(this)
#include "server/event_loop.hpp"

namespace server{

Acceptor::Acceptor(EventLoop* loop, const struct sockaddr_in& addr)
    : sock_(::socket(AF_INET, SOCK_STREAM, 0)),
      ch_(sock_.fd(), loop),
      loop_(loop)
{
    // 注意初始化顺序：成员按声明顺序构造，sock_ 先于 ch_，故 ch_ 构造时 sock_.fd() 已确定
    if(!sock_.vaild()){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "Acceptor socket() failed:", eg.message());
        return;  // 后续 listen() 靠 vaild() 拦下
    }

    int one = 1;
    ::setsockopt(sock_.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));  // 调试方便规避 TIME_WAIT

    // 监听 fd 必须非阻塞（ET accept 的前提），只设一次
    int flags = ::fcntl(sock_.fd(), F_GETFL, 0);
    if(flags == -1 || ::fcntl(sock_.fd(), F_SETFL, flags | O_NONBLOCK) == -1){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "Acceptor fcntl(O_NONBLOCK) failed:", eg.message());
        return;
    }

    if(::bind(sock_.fd(), reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) < 0){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "Acceptor bind() failed:", eg.message());
        return;
    }
    if(::listen(sock_.fd(), 64) < 0){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "Acceptor listen() failed:", eg.message());
        return;
    }
}

void Acceptor::setNewConnectionCallback(std::function<void(int)> cb){
    newConnectionCallback_ = cb;
}

bool Acceptor::listen(){
    if(!sock_.vaild()) return false;   // socket/bind/listen 已失败，无可监听

    ch_.setreadback([this](){ this->onAccept(); });
    ch_.enablereading();  // 内部 update() → loop_->updatachannel 上树，单点发生

    return true;
}

void Acceptor::onAccept(){
    for(;;){
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int cfd = ::accept(sock_.fd(), reinterpret_cast<struct sockaddr*>(&cli), &len);
        if(cfd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;  // ET：把待 accept 队列读空
            ErrnoGuard eg;
            Logger::instance().log(Level::ERROR, "Acceptor accept() failed:", eg.message());
            break;
        }
        Logger::instance().log(Level::INFO, "accepted fd=", cfd);
        if(newConnectionCallback_)
            newConnectionCallback_(cfd);  // cfd 的所有权转交上层；上层不接管则负责关闭
    }
}

}