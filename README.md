# MyServer — 低延迟高性能 TCP 服务器

*Low-Latency High-Performance TCP Server*

> An event-driven, epoll-based TCP server in modern C++ (Muduo-style single-threaded Reactor core), built with an emphasis on clear architecture and readable layering before concurrency is added.
>
> 一个基于 epoll 事件驱动的现代 C++ TCP 服务器（Muduo 风格单线程 Reactor 核心）。先行搭建清晰架构与可读的分层，再逐步加入并发。

**Target / 目标环境:** Linux / [WSL2] (Ubuntu) · **Toolchain / 工具链:** g++ 13 · CMake ≥ 3.10

---

## 特性 / Highlights

- **Reactor 事件循环** — 单个 `EventLoop` 驱动一个 `EpollPoller`，把就绪的 `Channel*` 回传用于分发；循环与分发在 `EventLoop` 中完成，而不是在 poller 里。
- **清晰的资源所有权** — 资源由 RAII 封装（`Socket`）或连接对象（`Channel`、`TcpConnection`）持有；poller 只维护一个非占有的 fd → `Channel` 查找表。
- **刻意慢速分层** — 每个组件都获得独立职责后再进入下一层，保证骨架始终可审查。

> ✅ **Status / 状态: 单连接回显已跑通 — Reactor core + Acceptor + TcpConnection 可编译运行，端到端回显验证通过。**
> 单线程 Reactor 核心（poller / epollpoller / event_loop）、`Acceptor`（监听 + ET accept）、`TcpConnection`（连接生命周期 + 回显）均已实现；服务组装在 `main.cpp`。多线程尚未实现。见 [Route / 路线图](#route--路线图).

---

## 架构 / Architecture

```
┌─────────────────────────────────────────────┐
│                 main.cpp                    │  entry point
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                main.cpp                    │  entry point
│   Acceptor → accepts → TcpConnection(×N)   │  assembly: 监听 + 每连接 I/O（服务组装现驻 main）
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                EventLoop                    │  引擎：循环 + 分发  (owns a Poller)
│   poll() → for each ready ch: ch.handleback │
└───────────────┬─────────────────────────────┘
                │ updatachannel / removechannel
┌───────────────▼─────────────────────────────┐
│       Poller (抽象/abstract)                │
│       EpollPoller (epoll 实现)              │   epoll_wait → active Channel*
└─────────────────────────────────────────────┘
                    │ fd ↔ Channel
┌─────────────────────────────────────────────┐
│   Channel   (单 fd 事件分发 / 不拥有 fd)    │
│   Socket    (RAII fd 持有者 / 唯一 close 方) │  基础封装 base encap.
│   Buffer    (I/O 缓冲，规划中/planned)      │
└─────────────────────────────────────────────┘
```

设计遵循经典 **Reactor** 模式：单线程持有 epoll fd 并运行事件循环；就绪状态以 `Channel*` 回传，按事件分发回调。

---

## 目录结构 / Directory Layout

```
.
├── CMakeLists.txt
├── README.md
├── include/server/
│   ├── socket.hpp/.cpp      # RAII fd 封装 / fd wrapper
│   ├── channel.hpp/.cpp     # 单 fd 事件分发 / per-fd dispatch（持 fd 数字，不拥有）
│   ├── poller.hpp           # Poller 抽象 / abstraction
│   ├── epollpoller.hpp      # epoll 实现 / epoll backend
│   ├── event_loop.hpp/.cpp  # Reactor 引擎 / the engine
│   ├── tcp_connnection.hpp/.cpp # 单个连接管理（Socket+Channel） / one connection
│   ├── acceptor.hpp/.cpp    # 监听 + ET accept / listening & accepting
│   ├── log.hpp/.cpp         # Logger 单例 / logger singleton
│   ├── errno.hpp/.cpp       # errno RAII 守卫 / errno guard
│   └── (thread_pool.*)      # （规划中 / planned）
└── src/
    └── main.cpp             # 入口 + 服务组装 / entry & assembly
```

---

## 构建与运行 / Build & Run

```bash
cmake -S . -B build
cmake --build build
./build/server          # 监听 127.0.0.1:8888
# 另一终端验证回显：
printf 'hello\n' | nc 127.0.0.1 8888   # 期望回显 hello
```

---

## 路线图 / Roadmap

- [x] 基础封装 base wrappers: `Socket`, `Logger`, `ErrnoGuard`（`Noncopyable` 采用 `Socket` 的 delete 拷贝，无需单独基类）
- [x] Reactor 核心: `Channel`, `Poller` / `EpollPoller`, `EventLoop`
- [x] `Acceptor` — 监听 + ET accept，把 fd 交给 `TcpConnection`
- [x] `TcpConnection` I/O 回调 — 基于 epoll 的单连接回显（端到端验证通过）
- [x] 首个端到端演示：完整回显服务器
- [ ] `Buffer` — 让 read/write 与 socket 就绪状态解耦
- [ ] 「正常断开 vs 真错误」日志语义细分（FIN → INFO，仅真错误 → ERROR）
- [ ] `ThreadPool` + 多 Reactor 线程

---

## 贡献者说明 / Notes for contributors

- 代码遵循"小文件、清晰分层"原则。声明（`.hpp`）与实现（`.cpp`）已完全分离，规避 ODR 重复定义；新增组件按同样约定落位。
- poller 设计为抽象基类，epoll 后端可替换（例如 `select`、`poll`）而不影响 `EventLoop`。
- 注释与文档面向对事件驱动服务器尚不熟悉的读者书写。

[WSL2]: https://learn.microsoft.com/en-us/windows/wsl/