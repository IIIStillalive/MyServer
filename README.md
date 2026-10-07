# MyServer — 低延迟高性能 TCP 服务器

*Low-Latency High-Performance TCP Server*

> An event-driven, epoll-based TCP server in modern C++ (Muduo-style **multi-Reactor** core), unfolded in clear layers: single-thread echo → threading base → app buffer → transport protocol → safe teardown.
>
> 一个基于 epoll 事件驱动的现代 C++ TCP 服务器（Muduo 风格**多 Reactor** 核心），按清晰分层逐步展开：单线程回显 → 多线程地基 → 应用层缓冲 → 传输协议 → 安全析构。

**Target / 目标环境:** Linux / [WSL2] (Ubuntu) · **Toolchain / 工具链:** g++ 13 · CMake ≥ 3.10

---

## 特性 / Highlights

- **多 Reactor 并行** — 每个线程一个 `EventLoop`；`ThreadPool` + `getNextLoop()` 轮询把新连接派发到 worker loop；`runInLoop`/`eventfd` 跨线程安全派发任务。
- **应用层缓冲 `Buffer`** — `[readerIndex_, writerIndex_)` 未读模型；`readFd` 用 `readv` 双段减拷贝；read/write 与 socket 就绪状态解耦。
- **长度前缀协议 `LengthHeaderCodec`** — `[4 字节大端长度][payload]` 帧；`onMessage` 循环切帧，半包攒、粘包拆。
- **安全析构（UAF 免疫）** — `TcpConnection` 继承 `enable_shared_from_this`，回调栈内局部共享指针保活；连接容器 `conns_` 插入/删除全部串行化在 main loop 线程，消除跨线程 race。
- **优雅停机** — signal handler 仅做 atomic store + eventfd write（async-signal-safe），`quit_` 原子标志，Ctrl+C / SIGTERM 干净退出。
- **刻意逐层展开** — 每个组件拿到独立职责稳定后再进下一层，骨架始终可审查。

> ✅ **Status / 状态: 传输层已完整闭合 — ThreadPool 多线程接池 + Buffer + 长度前缀 Codec + shared_ptr 安全析构 + 连接容器串行化 + 优雅停机全部落地，`test/codec_client.py`（单条/粘包/半包）与 `test/concurrent_client.py`（50 连接 × 20 消息并发）验证通过。**

---

## 架构 / Architecture

```
┌─────────────────────────────────────────────┐
│                main.cpp                    │  entry point（挂业务回调 + 信号处理）
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                TcpServer                    │  服务组装 (owns ThreadPool)
│   Acceptor ──accepts──▶ TcpConnection(×N)  │  监听 + 每连接 I/O
│   LengthHeaderCodec  ◀──剥帧/编帧           │  传输协议
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│        EventLoopThread / ThreadPool         │  多 Reactor（每个 worker 一个 loop）
│        EventLoop (main)  └─getNextLoop─▶    │  
└───────────────┬─────────────────────────────┘
                │ updatachannel / removechannel / runInLoop(跨线程唤醒)
┌───────────────▼─────────────────────────────┐
│       Poller (抽象/abstract)                │
│       EpollPoller (epoll 实现)              │   epoll_wait → active Channel*
└─────────────────────────────────────────────┘
                    │ fd ↔ Channel
┌─────────────────────────────────────────────┐
│   Channel   (单 fd 事件分发 / 不拥有 fd)    │
│   Socket    (RAII fd 持有者 / 唯一 close 方) │  基础封装 base encap.
│   Buffer    (应用层 I/O 缓冲)               │
└─────────────────────────────────────────────┘
```

设计遵循经典 **Reactor** 模式：就绪状态以 `Channel*` 回传，按事件分发回调。`EventLoop` 用 `eventfd` 实现跨线程唤醒——任务（`runInLoop`）可被派到任意线程的 loop 执行；`ThreadPool` 轮询挑一个 worker loop 承接新连接，每个连接的 buffer 只在其归属 loop 线程内访问。

---

## 目录结构 / Directory Layout

```
.
├── CMakeLists.txt
├── README.md
├── include/server/
│   ├── socket.hpp/.cpp          # RAII fd 封装 / fd wrapper
│   ├── channel.hpp/.cpp         # 单 fd 事件分发 / per-fd dispatch（持 fd 数字，不拥有）
│   ├── poller.hpp               # Poller 抽象 / abstraction
│   ├── epollpoller.hpp          # epoll 实现 / epoll backend
│   ├── event_loop.hpp/.cpp      # Reactor 引擎 + 跨线程唤醒（runInLoop/eventfd） / the engine
│   ├── eventloopthread.hpp/.cpp # 每线程一个 loop 的启动器 / run one loop per thread
│   ├── thread_pool.hpp/.cpp     # N 个 worker loop + getNextLoop() 轮询 / multi-reactor pool
│   ├── buffer.hpp/.cpp          # 应用层缓冲（readv 减拷贝） / app-level buffer
│   ├── length_header_codec.hpp/.cpp # 长度前缀编解码（[4B 长度][payload]） / framing codec
│   ├── tcp_connnection.hpp/.cpp # 单个连接管理（Socket+Channel+Buffer） / one connection
│   ├── acceptor.hpp/.cpp        # 监听 + ET accept / listening & accepting
│   ├── tcpserver.hpp/.cpp       # 服务组装（持有连接 + 线程池 + codec） / server assembly
│   ├── log.hpp/.cpp             # Logger 单例 / logger singleton
│   ├── errno.hpp/.cpp           # errno RAII 守卫 / errno guard
└── src/
    └── main.cpp                 # 入口 + 业务回调 + 优雅停机 / entry & assembly
```

---

## 构建与运行 / Build & Run

```bash
cmake -S . -B build
cmake --build build
./build/server          # 监听 127.0.0.1:8888
```

⚠️ **协议是长度前缀帧**：启用 Codec 后，裸 `nc` 发送**不再是原样回显**，需用带长度头的编码客户端收发。最小验证客户端：

```python
import socket, struct

def encode(msg: bytes) -> bytes:
    return struct.pack(">I", len(msg)) + msg     # [4字节大端长度][payload]

s = socket.create_connection(("127.0.0.1", 8888))
s.sendall(encode(b"hello"))                      # 发送帧
ln = struct.unpack(">I", s.recv(4))[0]           # 读回长度头
print(s.recv(ln))                                # 期望 b'hello'
```

---

## 路线图 / Roadmap

- [x] 基础封装 base wrappers: `Socket`, `Logger`, `ErrnoGuard`
- [x] Reactor 核心: `Channel`, `Poller` / `EpollPoller`, `EventLoop`
- [x] `Acceptor` — 监听 + ET accept，把 fd 交给 `TcpConnection`
- [x] `TcpConnection` I/O 回调 — 基于 epoll 的单连接回显（端到端验证通过）
- [x] `TcpServer` — 服务组装（持有连接 + 释放）
- [x] EventLoop 跨线程地基 — `eventfd` 唤醒 + `runInLoop` + `EventLoopThread`
- [x] `ThreadPool` — N 个 worker loop + `getNextLoop()` 轮询（numThreads==0 降级回 main loop）
- [x] TcpServer 接池 — 新连接派发到 worker loop，连接容器按 loop 分区、串行化在 main loop
- [x] `Buffer` — 应用层读缓冲 + 写缓冲，readv 减拷贝
- [x] `LengthHeaderCodec` — 长度前缀编解码，半包/粘包正确切帧
- [x] `TcpConnection` shared_ptr 安全析构 — 回调栈保活，防 UAF / fd 泄漏
- [x] 优雅停机 — signal + atomic quit 标志，async-safe handler
- [x] 连接容器串行化 — `conns_` map 消除跨线程 race
- [ ] 真实业务 — 转发 / 广播 / 聊天室（替换 echo 桩）
- [ ] `TimerQueue` — 定时器（心跳 / 超时关闭）
- [ ] 「正常断开 vs 真错误」日志语义细分（FIN → INFO，仅真错误 → ERROR）

---

## 贡献者说明 / Notes for contributors

- 代码遵循"小文件、清晰分层"原则。声明（`.hpp`）与实现（`.cpp`）已完全分离，规避 ODR 重复定义；新增组件按同样约定落位。
- poller 设计为抽象基类，epoll 后端可替换（例如 `select`、`poll`）而不影响 `EventLoop`。
- **回/转发铁律**：经 Codec 收到的 payload 是已剥头数据，回复时**必须再经同一 Codec 编码长度头**（`TcpServer::sendMessage`），不能裸 `conn->send(payload)`——否则对端把 payload 前 4 字节当长度，解出错误帧。
- **shared_ptr 保活铁律**：`TcpConnection` 的保活 shared_ptr 只能作回调栈内局部变量；绝不可捕获进对象的成员（尤其 close 回调），否则形成自引用循环 → 对象永不析构 → fd 永不关 → 连接泄漏。
- 注释与文档面向对事件驱动服务器尚不熟悉的读者书写。

[WSL2]: https://learn.microsoft.com/en-us/windows/wsl/