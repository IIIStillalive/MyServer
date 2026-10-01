#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include "event_loop.hpp"

namespace server {

class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();

    // 启动线程，并在该线程中创建 EventLoop，最后返回该 EventLoop 的指针
    EventLoop* startLoop();
    void quit();

private:
    void threadFunc(); // 线程的入口函数

    EventLoop* loop_;                  // 指向子线程中创建的 EventLoop 对象
    bool exiting_;                     // 是否正在退出
    std::thread thread_;               // 封装的底层线程
    std::mutex mutex_;                 // 保护 loop_ 指针的互斥锁
    std::condition_variable cond_;     // 用于同步等待 loop 创建完成的条件变量
};

}