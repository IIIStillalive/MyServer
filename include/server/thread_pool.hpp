#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

namespace server {

class ThreadPool {
public:
    using Callback = std::function<void()>; 
    
    // 推荐在构造时就传入工作回调，或者提供默认行为
    ThreadPool(size_t threadNum, Callback workCallback);
    ~ThreadPool();

    void stop();

private:
    size_t threadNum_;                      // 修正重名：工作线程数量
    Callback workCallback_;                 // 工作线程的主循环回调
    std::vector<std::thread> workers_;      // 线程容器
    std::atomic<bool> stop_;                // 停止标志（建议用 atomic 保证多线程可见性）
};

// --- 实现部分 ---

ThreadPool::ThreadPool(size_t threadNum, Callback workCallback)
    : threadNum_(threadNum), 
      workCallback_(workCallback), 
      stop_(false) 
{
    for(size_t i = 0; i < threadNum_; ++i){
        // 在这里启动线程，并把 workCallback_ 传进去
        workers_.emplace_back(workCallback_);
    }
}

ThreadPool::~ThreadPool() {
    stop();
    for(auto& t : workers_){
        if(t.joinable()){
            t.join(); // 确保线程安全退出
        }
    }
}

void ThreadPool::stop() {
    stop_ = true;
}

} // namespace server