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
#include "timer.hpp"   // TimerCallback / TimerId / nowMicros
namespace server{  //为readys调用回调函数
class Channel;
class Poller;
class TimerQueue;
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

    // ---- 定时器接口（要求调用者处于本 loop 线程；跨线程由调用方 runInLoop 转发）----
    TimerId runAfter(double delay, TimerCallback cb);    // delay 秒后执行一次
    TimerId runEvery(double interval, TimerCallback cb); // 每 interval 秒循环
    void cancel(TimerId id);                             // 取消未到期定时器


private:
    int createEventfd();  //初始化eventfd_
    bool handlewake();  //处理唤醒事件;
    void dopendingFunctors();


    std::vector<Channel*> readys;
    //Poller* poller_;  //指向子类对象
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;  //timerfd 定时队列（归本 loop 线程）
    std::thread::id thread_id_;  //当前loop的归属Thread的id

    std::mutex mutex_;
    Socket eventsoc_;  //负责唤醒线程的fd；
    std::unique_ptr<Channel> eventch_;  //负责eventfd状态设置和改变的channel
    std::vector<Callback> pendingFunctors_;  //任务队列

    std::atomic<bool> quit_{false};  //loop结束标志（跨线程 quit 须原子；signal handler 里也要写）
};
}