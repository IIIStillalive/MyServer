#pragma once

#include <cstdint>
#include <functional>

//事件分发
namespace server{

class EventLoop;  // 前向声明，避免 include event_loop.hpp 造成循环依赖

class Channel{

public:
    using Callback = std::function<void()>;
    explicit Channel(int fd, EventLoop* loop);
    ~Channel() = default;  //不持有任何资源,于是析构函数什么都不做

    void enablereading();
    void enablewriting();  //告诉epoll这个事件关注什么

    //void disablereading();  //
    void disablewriting();  //在完成某个事件后取消
    bool isWriting() const;  //当前是否处于可写关注（TcpConnection 发送前判断）
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

}