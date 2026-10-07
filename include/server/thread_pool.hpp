#pragma once

#include <memory>
#include <vector>

namespace server {

class EventLoopThread;
class EventLoop;

// IO 事件循环池：持有 N 个 EventLoopThread，每个各跑一个 EventLoop。
//   用途：TcpServer 收到新连接时 getNextLoop() 轮询取一个 worker loop，
//         把连接派到该 loop 的线程里做上树与 I/O（多 Reactor 并行）。
//   约定：线程数被设为 0 时，getNextLoop() 降级返回 mainLoop（单线程运行）。
class ThreadPool {
public:
    explicit ThreadPool(EventLoop* mainLoop);
    ~ThreadPool();

    void setThreadNum(size_t n);   // 设置 worker 数量（须在 start 前调用）
    void start();                  // 起 n 个 EventLoopThread，收集其 loop 指针
    EventLoop* getNextLoop();      // 轮询返回下一个 worker loop；线程数 0 时返回 mainLoop

    std::vector<EventLoop*> getAllLoops();  // 所有 worker loop（不含 mainLoop）

private:
    EventLoop* mainLoop_;                        // TcpServer 的主 loop（非 worker）
    size_t threadNum_ = 0;                       // worker 线程数量
    std::vector<std::unique_ptr<EventLoopThread>> threads_;  // 线程容器（owner）
    std::vector<EventLoop*> loops_;              // 每个 EventLoopThread 返回的 loop 指针
    size_t next_ = 0;                            // 轮询游标（仅在 mainLoop 线程访问）
};

} // namespace server