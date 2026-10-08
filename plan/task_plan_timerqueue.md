# TimerQueue 定时器 — 实现计划

状态：`[已完成，WSL 运行验证通过]`

## 目标 / Scope
给 EventLoop 增加定时能力（单次 `runAfter` + 循环 `runEvery` + `cancel`），
用 **timerfd** 驱动 epoll，不与 eventfd 唤醒环冲突。

## 设计原理（结论优先）
- **为什么 timerfd 而非 poll 超时**：`poller->poll(-1)` 无限等待已跑通，不该把"超时计算"
  塞回 poller；timerfd 是一个普通 fd，变可读时走既有 Channel 分发 → 最小改动、复用现有
  epoll 回路、微秒级精度、`CLOCK_MONOTONIC` 抗系统时间跳变。
- **每 loop 一个 TimerQueue**：timerfd 必须在目标 loop 的 poller 上注册，故 TimerQueue 属于
  一个 EventLoop。Timer 回调在归属 loop 线程跑（与本项目"每连接 buffer 只归其 loop"一致）。
- **一版只做同线程调度**：`runAfter/runEvery` 要求调用者在目标 loop 线程；跨线程由调用方
  `runInLoop` 转发（与 Muduo 有 gap，作为已知边界，够当前业务用）。

## 数据结构
- `Timer`：`expiry_`(绝对微秒) + `interval_`(重复间隔, 0=单次) + `callback_` + `sequence_`
  (全局递增 id，同刻 tie-break + 作 cancel 句柄)。
- `TimerQueue`：`std::set<Timer*, TimerCmp{expiry,seq}> timers_`（最早在前）。
  - `timerfd` 设成 set 里**最早**到期时刻；到期可读一次走 handleRead。
  - `cancel(id)`：遍历 set 删除（回调内不取消防患自己，作为第一版边界说明）。

## 待办（P0）
- [x] timer.hpp — Timer 节点 + 比较器 + nowMicros()
- [x] timer_queue.hpp/.cpp — addTimer / handleRead / getExpired / resetTimerfd / cancel
- [x] event_loop.hpp/.cpp — 挂 timerQueue_ + runAfter/runEvery/cancel 转发
- [x] CMakeLists — 加 timer_queue.cpp；新增 timer_demo 目标
- [x] src/timer_demo.cpp — 独立演示：单次 + 循环 + cancel + 自动 quit
- [x] WSL 编译 + 跑 demo 验证（已完成：tick×3 → cancel → 3.0s quit，全程无 ERROR；EAGAIN 误报已修）

## 验证
- demo 期望输出顺序：every 0.5s 三次 → 1.6s once cancel → 循环停 → 3.0s quit
- 可选：server 上临时 runEvery 低频 tick，与 concurrent_client 并发共存观察

## 已踩坑
- **TFD_NONBLOCK + ET 下 `read` 返回 EAGAIN 是正常态**：计数已被前一次消费 / 重装竞态，残留
  可读会再报一次，此时非阻塞读返回 -1/EAGAIN。若当 ERROR 打印会成串刷屏且提前 return。修法：
  `read<0 且 errno∈{EINTR,EAGAIN,EWOULDBLOCK}` → 静默返回，让下一次触发接手。与 eventfd 唤醒环
  `handlewake` 处理的 EAGAIN 是同一类问题（f018e84 定位 ET 读取时已建立该容忍模式）。

## 边界 / 不做（P2）
- 跨线程 run* 调度（第一版要求同线程）
- 回调内取消"当前这轮重复 timer"自身（UAF 风险点，文档注明，不做 canceling 机制）
- 单 loop 内定时任务的优先级/公平性（无）