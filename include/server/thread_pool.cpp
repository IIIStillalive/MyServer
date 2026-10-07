#include "thread_pool.hpp"

#include "eventloopthread.hpp"  // EventLoopThread 完整定义（unique_ptr 析构需完整类型）

namespace server {

ThreadPool::ThreadPool(EventLoop* mainLoop)
    : mainLoop_(mainLoop) {}

// unique_ptr 容器析构需 EventLoopThread 完整类型 —— 放 .cpp（同 EventLoop 的作法）
ThreadPool::~ThreadPool() = default;

void ThreadPool::setThreadNum(size_t n){
    threadNum_ = n;
}

void ThreadPool::start(){
    loops_.clear();
    threads_.clear();
    for(size_t i = 0; i < threadNum_; ++i){
        auto t = std::make_unique<EventLoopThread>();
        EventLoop* l = t->startLoop();   // 阻塞至子线程创建好 loop 并返回指针
        loops_.push_back(l);
        threads_.push_back(std::move(t));  // 先持有 loop 再存线程，避免丢指针
    }
}

EventLoop* ThreadPool::getNextLoop(){
    if(loops_.empty()) return mainLoop_;   // 线程数 0 → 降级回主 loop（单线程）
    EventLoop* l = loops_[next_];
    next_ = (next_ + 1) % loops_.size();   // 轮询
    return l;
}

std::vector<EventLoop*> ThreadPool::getAllLoops(){
    return loops_;
}

} // namespace server