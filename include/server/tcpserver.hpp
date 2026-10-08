#pragma once

#include <map>
#include <memory>
#include <netinet/in.h>   // struct sockaddr_in

#include "acceptor.hpp"
#include "tcp_connnection.hpp"
#include "thread_pool.hpp"
#include "length_header_codec.hpp"

namespace server{

class EventLoop;  // 前向声明；仅存指针，无需完整类型

// TcpServer：面向使用者的"服务器"门面（Muduo 风格）。
//   持有：
//     - Acceptor：监听 listen fd
//     - ThreadPool：N 个 worker，每个各跑一个 EventLoop（多 Reactor）
//     - conns_：全部活动连接的单一映射（key=fd），仅由 main loop 线程读写
//   连接生命周期：
//     - accept 到 cfd → getNextLoop() 轮询挑 worker → ioLoop->runInLoop 里创建连接并上树
//     - 连接关闭 → close 回调拉回 main loop 线程 → removeConnection 从映射移除
//   析构顺序（Muduo 关键）：threadPool_ 最后构造 → 最先析构 → worker 先停，连接后关
class TcpServer{

public:
    TcpServer(EventLoop* loop, const struct sockaddr_in& addr);
    ~TcpServer() = default;  // 触发 ~threadPool_ → 各 worker loop quit + join

    using MessageCallback = LengthHeaderCodec::MessageCallback;  // (TcpConnection*, const std::string&)
    void setThreadNum(size_t n);   // 透传给 ThreadPool：worker 数量
    void setMessageCallback(const MessageCallback& cb);  // 业务层：收到一条完整消息
    void setConnectionTimeout(double seconds); // 透传：每连接空闲超时（0 或未调用=不启用）
    void sendMessage(TcpConnection* conn, const void* data, size_t n);  // 编码长度头后发给某连接
    bool start();                  // 先起线程池，再建 Acceptor + 挂 epoll

private:
    void onNewConnection(int cfd);      // Acceptor 回调；在 main loop 线程
    void removeConnection(int cfd);     // 从 conns_ 移除并释放；在 main loop 线程

    EventLoop* loop_;                   // main loop（Acceptor + 连接容器所在）
    Acceptor acceptor_;                 // 监听方（持有 listen fd）
    ThreadPool threadPool_;             // worker 池（值成员，最后构造→最先析构）
    std::map<int, std::shared_ptr<TcpConnection>> conns_;  // key=fd，单一映射；shared_ptr 供回调栈保活
    LengthHeaderCodec codec_;            // 长度前缀编解码：粘包/半包 → 一条完整业务消息
    double connectionTimeout_ = 0;       // 空闲超时（秒），0=不启用；onNewConnection 时透传给新连接
};

}