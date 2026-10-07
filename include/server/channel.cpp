#include "channel.hpp"

#include <sys/epoll.h>

#include "server/event_loop.hpp"  // EventLoop 完整定义：update() 里调 loop_->updatachannel

namespace server{

Channel::Channel(int fd, EventLoop* loop): fd_(fd), events(0), loop_(loop), revents_(0) {}

void Channel::enablereading(){
    events |= EPOLLIN | EPOLLET;  //追加而不是覆盖
    update();
}
void Channel::enablewriting(){
    events |= EPOLLOUT | EPOLLET;
    update();
}
void Channel::update(){
    loop_->updatachannel(this);
}

void Channel::disablewriting(){
    this->events &= ~EPOLLOUT;
    update();  //清位后必须同步到 epoll，否则 EPOLLOUT 仍被报告 → 空转/忙等
}
bool Channel::isWriting() const{
    return events & EPOLLOUT;
}

void Channel::handleback(){
    if(revents_ & EPOLLHUP){
        if(closecallback){
            closecallback();  // 对端挂断 → 独立关闭回调（正常断开走 INFO，不按错误处理）
        }
    }
    if(revents_ & EPOLLERR){
        if(errorcallback){
            errorcallback();  // 真错误 → 按 error 处理
        }
    }
    if(revents_ & EPOLLIN)
    {
        if(readcallback){
            readcallback();
        }
    }
    if(revents_ & EPOLLOUT){
        if(writecallback){
            writecallback();
        }
    }
}


int Channel::fd() const{
    return fd_;
}
uint32_t Channel::_events() const{
    return this->events;
}
bool Channel::isremove() const{
    return this->remove;
}

void Channel::setcloseback(Callback cb){
    closecallback = cb;
}
void Channel::setreadback(Callback cb){
    readcallback = cb;
}
void Channel::setwriteback(Callback cb){
    writecallback = cb;
}
void Channel::seterrorback(Callback cb){
    errorcallback = cb;
}
void Channel::setrevents(uint32_t revents){
    revents_ = revents;
}
}