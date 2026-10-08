#pragma once

#include <functional>
#include <memory>

#include "buffer.hpp"

#include "channel.hpp"
#include "socket.hpp"
#include "timer.hpp"   // TimerId：closeTimerId_ 归属类型
namespace server{

class Buffer;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>{  //管理一整个连接

public:

    TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection() = default;

    bool listenconnection();
    bool connection();

    int fd();
    EventLoop* getLoop() const;  // 归属 loop：释放时把所有权交回该线程析构，消除跨线程竞态
    void loop();  // 工作循环

    // 预留回调接口：连接关闭时通知所有者(main)把我释放；所有权属于外部
    void setCloseCallback(std::function<void(TcpConnection*)> cb);
    void send(const void* data, size_t n);   // public：业务侧发送入口
    using MessageCallback = std::function<void(TcpConnection*, Buffer*)>;
    void setMessageCallback(MessageCallback cb);   // 业务层挂钩：onRead 读到的字节交给它解析
    void setConnectionTimeout(double seconds);     // 启动空闲超时：超时未收发数据则主动断开

private:
    void onWrite();                          // 可写事件回调
    void writeNow();                         // 把 outputBuffer_ 尽力发一截
    Buffer inputBuffer_;                     // 输入缓冲
    Buffer outputBuffer_;                    // 输出缓冲（待发累积）

    void handleClose();
    void handleError();
    void onRead();
    void onError();
    int set_nonblocking();
    Socket sock_;  //持有socket资源
    Channel ch_;  //持有channel资源，负责分发
    EventLoop* loop_;  //持有一个loop指针, 负责epoll事务
    void checkIdle();                        // 心跳定时器回调：超时未活跃 → handleClose()

    std::function<void(TcpConnection*)> closeCallback_;  // 点对点的释放通知
    MessageCallback messageCallback_;                    // 连接层读到字节后的业务分发
    bool closed_ = false;  // 防重复关闭
    int64_t timeoutUs_ = 0;        // 空闲超时阈值（微秒）；仅 ioLoop 线程读
    int64_t lastActive_ = 0;       // 最后活跃时刻（CLOCK_MONOTONIC 微秒）；仅 ioLoop 线程读写
    TimerId timeoutTimerId_ = 0;   // 心跳定时器 id，handleClose 时取消防悬垂

};

}