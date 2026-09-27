#pragma once
#include <vector>

namespace server{  //为readys调用回调函数
class Channel;
class Poller;
class EventLoop{

public:
    EventLoop();
    ~EventLoop() = default;

    void updatachannel(Channel* ch);  //调用poller来更改状态
    void removechannel(Channel* ch);  //调用poller来移除监听

    void runloop();


private:
    std::vector<Channel*> readys;
    Poller* poller_;  //指向子类对象
    //std::unique_ptr<Poller> poller_;

};
}