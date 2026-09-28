#pragma once
#include <memory>
#include <vector>

namespace server{  //为readys调用回调函数
class Channel;
class Poller;
class EventLoop{

public:
    EventLoop();
    ~EventLoop();  // 定义在 .cpp：unique_ptr<Poller> 析构需 Poller 完整类型

    bool updatachannel(Channel* ch);  //调用poller来更改状态，返回是否成功
    void removechannel(Channel* ch);  //调用poller来移除监听

    void runloop();


private:
    std::vector<Channel*> readys;
    //Poller* poller_;  //指向子类对象
    std::unique_ptr<Poller> poller_;

};
}