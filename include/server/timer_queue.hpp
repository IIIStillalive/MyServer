#pragma once

#include <stdint.h>
#include <set>
#include <vector>

#include "timer.hpp"
#include "channel.hpp"
#include "socket.hpp"

namespace server {
class EventLoop;

// timerfd 驱动的定时队列：只在其归属 loop 线程内使用（一版不跨线程调度）
class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // 登记一个定时器（expiry=绝对微秒，interval=0 单次 / >0 重复）
    // 返回 TimerId 供 cancel
    TimerId addTimer(TimerCallback cb, int64_t expiry, int64_t interval);

    void cancel(TimerId id);   // 取消尚未到期的定时器；回调内取消"当前这轮重复 timer"不算

private:
    void handleRead();                         // timerfd 可读 → 取出到期批
    void getExpired(std::vector<Timer*>& expired, int64_t now); // 到期<=now 的全部移出
    void resetTimerfd();                       // 把 timerfd 设成 set 最早到期时刻

    std::set<Timer*, TimerCmp> timers_;        // 有序，最早在前（TimerQueue 持有所有权）
    EventLoop* loop_;
    Socket     timerfd_;                       // RAII 持 timerfd fd
    Channel    channel_;                       // timerfd 的 channel，注册到 loop_->poller_
    int64_t    earliestExpiry_ = 0;            // 当前 timerfd 已设到期时刻，避免无谓重设
    uint64_t   nextSequence_ = 0;              // 全局递增 TimerId 源
};

} // namespace server