#include "tcp_connnection.hpp"

#include <fcntl.h>
#include <unistd.h>
#include "server/log.hpp"
#include <thread>
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
int TcpConnection::fd(){
    return sock_.fd();
}

// onRead：用 inputBuffer_ 读，回显走 send（不再裸 write）
void TcpConnection::onRead(){
    std::shared_ptr<TcpConnection> keep = shared_from_this();  // 保活：回调栈结束前不许被析构(UAF 免疫)
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(sock_.fd(), &saveErrno);
    if(n > 0){
        if(messageCallback_) messageCallback_(this, &inputBuffer_);  // 交给解析层(Codec)；不再硬编码回显
    } else if(n == 0){
        handleClose();
    } else {
        if(saveErrno != EAGAIN && saveErrno != EWOULDBLOCK)
            handleError();
        // EAGAIN → 读空，本轮正常结束
    }
}

// send：能直写就先直写，写不完的进 outputBuffer_ 挂可写
void TcpConnection::send(const void* data, size_t n){
    if(ch_.isremove()) return;
    const char* raw = static_cast<const char*>(data);
    size_t remaining = n;
    if(!ch_.isWriting() && outputBuffer_.readableBytes() == 0){  // 无积压才直写
        ssize_t nwrote = ::write(sock_.fd(), raw, n);
        if(nwrote >= 0){
            remaining = n - static_cast<size_t>(nwrote);
            if(remaining == 0) return;        // 一次发完，不需要缓冲
        } else if(errno == EPIPE || errno == ECONNRESET){
            return handleClose();             // 对端已关
        }
        // EAGAIN → remaining 保持 n，全进缓冲
    }
    if(remaining > 0){
        outputBuffer_.append(raw + (n - remaining), remaining);
        if(!ch_.isWriting()) ch_.enablewriting();  // 积压 → 挂 EPOLLOUT 补发
    }
}

void TcpConnection::onError(){
    std::shared_ptr<TcpConnection> keep = shared_from_this();  // 保活
    Logger::instance().log(Level::ERROR, "write or read failed\n");
    handleClose();                              // 统一走关闭
}
void TcpConnection::writeNow(){  //把输出缓冲尽力发到内核缓冲满为止
    while(outputBuffer_.readableBytes() > 0){
        ssize_t n = ::write(sock_.fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if(n > 0){
            outputBuffer_.retrieve(static_cast<size_t>(n));
        }
        else if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            break;                              // 内核缓冲满 → 保持可写，下轮补发
        }
        else{
            handleError();
            return;
        }
    }
    if(outputBuffer_.readableBytes() == 0)
        ch_.disablewriting();                   // 发空 → 关可写（同步到 epoll 才真停）
}
void TcpConnection::onWrite(){  // EPOLLOUT 可写事件回调
    std::shared_ptr<TcpConnection> keep = shared_from_this();  // 保活
    if(ch_.isremove()) return;
    writeNow();
}

bool TcpConnection::listenconnection(){
    if(set_nonblocking() < 0) return false;  //设置为非堵塞
    ch_.setreadback([this](){ this->onRead();});
    ch_.setcloseback([this](){ this->handleClose();});  // 对端挂断 → 关闭（INFO，不按错误）
    ch_.setwriteback([this](){ this->onWrite();});
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
void TcpConnection::setMessageCallback(MessageCallback cb){
    messageCallback_ = cb;
}
void TcpConnection::handleClose(){
    if(closed_) return;                  // 防重复关闭（HUP 可能会触达多次）
    closed_ = true;
    Logger::instance().log(Level::INFO, "threadID: ", std::this_thread::get_id());
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