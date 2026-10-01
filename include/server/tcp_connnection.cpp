#include "tcp_connnection.hpp"

#include <fcntl.h>
#include <unistd.h>
#include "server/log.hpp"

#include "server/event_loop.hpp"  // EventLoop 完整定义：listenconnection() 里调 loop_->updatachannel

namespace server{

TcpConnection::TcpConnection(int fd, EventLoop* loop): sock_(fd), ch_(fd, loop), loop_(loop){}
int TcpConnection::set_nonblocking(){
    int fd = sock_.fd();
    int flags = ::fcntl(fd, F_GETFL, 0);
    if(flags == -1) return -1;
    if(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}
void TcpConnection::onRead(){
    char buf[8192];
    while(true){
        int n = ::read(sock_.fd(), buf, sizeof(buf));
        if(n > 0){
            ::write(sock_.fd(), buf, n);   // 回显；有数据就处理
        }
        else if(n == 0){
            handleClose();                  // 对端关闭 → 摘除 + 标记
            break;                          // 必须出循环
        }
        else{  // n < 0
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;                      // 读空，正常结束本次事件
            handleError();                  // 真错误
            break;                          // 也要出循环
        }
    }
}
void TcpConnection::onError(){
    Logger::instance().log(Level::ERROR, "write or read failed\n");
    handleClose();                              // 统一走关闭
}

bool TcpConnection::listenconnection(){
    if(set_nonblocking() < 0) return false;  //设置为非堵塞
    ch_.setreadback([this](){ this->onRead();});
    ch_.seterrorback([this](){ this->onError();});
    ch_.enablereading();   // 内部已调用 update() → updatachannel 上树，单点发生
    return true;
}

bool TcpConnection::connection(){

    return true;

}
void TcpConnection::setCloseCallback(std::function<void(TcpConnection*)> cb){
    closeCallback_ = cb;
}
void TcpConnection::handleClose(){
    if(closed_) return;                  // 防重复关闭（HUP 可能会触达多次）
    closed_ = true;
    Logger::instance().log(Level::INFO, "fd: ", sock_.fd(), " closed, cleaning");
    loop_->removechannel(&ch_);          // ① 下树：poller 不再引用 ch_，防悬空
    if(closeCallback_) closeCallback_(this);  // ② 通知所有者 main 把我释放；此后别再用 this
}
void TcpConnection::handleError(){
    if(closed_) return;                  // 防重复：事件可能多次触达，只处理一次（与 handleClose 一致）
    Logger::instance().log(Level::ERROR, "conn error, closing");
    handleClose();
}


}