#pragma once

#include <stdint.h>
#include <functional>
#include <ctime>       // clock_gettime / CLOCK_MONOTONIC（nowMicros）

// 定时器节点 + 时间工具（TimerQueue 的存储单元）
namespace server {

using TimerCallback = std::function<void()>;
using TimerId = uint64_t;   // 全局递增，作为 set 同刻 tie-break 与 cancel 句柄

// 绝对到期时刻（微秒，CLOCK_MONOTONIC 基底）
inline int64_t nowMicros(){
    timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

class Timer {
public:
    Timer(TimerCallback cb, int64_t expiry, int64_t interval, uint64_t seq)
        : callback_(std::move(cb)), expiry_(expiry), interval_(interval), sequence_(seq) {}

    int64_t expiry()   const { return expiry_; }
    int64_t interval() const { return interval_; }
    uint64_t sequence() const { return sequence_; }
    bool repeat() const { return interval_ > 0; }

    void run() const { if(callback_) callback_(); }
    void restart(int64_t now){ expiry_ = now + interval_; }  // repeat：重算下次到期

private:
    TimerCallback callback_;
    int64_t expiry_;       // 绝对到期（微秒）
    int64_t interval_;     // 重复间隔（微秒，0 = 单次）
    uint64_t sequence_;    // 全局递增 id
};

// 比较器：先 expiry，同刻再用 sequence 分隔 → set 满足严格弱序且唯一
class TimerCmp {
public:
    bool operator()(const Timer* a, const Timer* b) const{
        if(a->expiry() != b->expiry()) return a->expiry() < b->expiry();
        return a->sequence() < b->sequence();
    }
};

} // namespace server