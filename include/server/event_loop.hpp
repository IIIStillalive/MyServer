#pragma once
#include <memory>
#include <vector>
#include <thread>
#include <functional>
#include <sys/eventfd.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include "socket.hpp"
namespace server{  //为readys调用回调函数
class Channel;
class Poller;
class EventLoop{

public:
    using Callback = std::function<void()>;
    EventLoop();
    ~EventLoop();  // 定义在 .cpp：unique_ptr<Poller> 析构需 Poller 完整类型

    bool updatachannel(Channel* ch);  //调用poller来更改状态，返回是否成功
    void removechannel(Channel* ch);  //调用poller来移除监听
    bool isInLoopThread() const;  //判断当下loop是否在自己的线程中
    void runloop();

    bool wakeup();  //唤醒其他线程Loop的接口
    void runInLoop(Callback cb);  //放入任务队列

    void quit();


private:
    int createEventfd();  //初始化eventfd_
    bool handlewake();  //处理唤醒事件;
    void dopendingFunctors();


    std::vector<Channel*> readys;
    //Poller* poller_;  //指向子类对象
    std::unique_ptr<Poller> poller_;
    std::thread::id thread_id_;  //当前loop的归属Thread的id

    std::mutex mutex_;
    Socket eventsoc_;  //负责唤醒线程的fd；
    std::unique_ptr<Channel> eventch_;  //负责eventfd状态设置和改变的channel
    std::vector<Callback> pendingFunctors_;  //任务队列

    std::atomic<bool> quit_{false};  //loop结束标志（跨线程 quit 须原子；signal handler 里也要写）
};
}