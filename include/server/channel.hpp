#pragma once
#include <cstdint>
#include <functional>
#include <sys/epoll.h>
#include "server/event_loop.hpp"



//事件分发
namespace server{

class Channel{

public:
    using Callback = std::function<void()>;
    explicit Channel(int fd, EventLoop* loop);
    ~Channel() = default;  //不持有任何资源,于是析构函数什么都不做

    void enablereading(); 
    void enablewriting();  //告诉epoll这个事件关注什么

    //void disablereading();  //
    void disablewriting();  //在完成某个事件后取消
    void update();  //更新状态

    void handleback();  //根据revents调用不同的callback,具体实现就是位操作

    int fd() const; 
    uint32_t _events() const;  //对外暴露的接口
    bool isremove() const; //向外界提供信息是否析构，防止自己析构自己，将析构交给上层

    void setcloseback(Callback cb);
    void setreadback(Callback cb);
    void setwriteback(Callback cb);
    void seterrorback(Callback cb);  //根据 epoll_wait 返回的struct epoll_event ev.events字段来给channel分发不同的回调函数
    void setrevents(uint32_t revents);

private:

    int fd_;
    uint32_t events;
    uint32_t revents_;
    bool remove = false; // 防止自己析构自己
    EventLoop* loop_; //向EventLoop发送自己状态信息改变的消息;


    Callback readcallback, writecallback, closecallback, errorcallback;
    
};

Channel::Channel(int fd, EventLoop* loop): fd_(fd), events(-1), loop_(loop), revents_(-1) {}

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