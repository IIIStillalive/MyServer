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
}

void Channel::handleback(){
    if(revents_ & (EPOLLERR | EPOLLHUP)){  //优先调用errorcallback()
        if(errorcallback){
            errorcallback();  //存在就调用
        }
    }
    else if(revents_ & EPOLLIN)
    {
        if(readcallback){
            readcallback();
        }
    }
    else if(revents_ & EPOLLOUT){
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