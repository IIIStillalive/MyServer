# MyServer — Low-Latency High-Performance TCP Server

An event-driven, epoll-based TCP server in modern C++ (Muduo-style single-threaded Reactor core), being built with a strong emphasis on clear architecture and readable layering before concurrency is added.

Target environment: Linux / [WSL2] (Ubuntu). Toolchain: g++ 13, CMake ≥ 3.10.

---

## Highlights

- **Reactor event loop** — one `EventLoop` drives an `EpollPoller`, which reports polled `Channel*` back for dispatch; loop and dispatch live in `EventLoop`, not in the poller.
- **Clean ownership** — resources are owned by RAII wrappers (`Socket`) or by the connection object (`Channel`, `TcpConnection`); the poller keeps a non-owning fd → `Channel` lookup map.
- **Slow, deliberate layering** — each component gets its own role before the next is added, so the skeleton stays reviewable.

> ⚠️ **Status: work in progress.** The Reactor core (channel / poller / epollpoller / event_loop) compiles and links. Service assembly (Acceptor, TcpConnection I/O callbacks) and the thread pool are not implemented yet. See [Roadmap](#roadmap).

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                 main.cpp                    │  entry point
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                TcpServer                    │  service assembly  (to be built)
│        Acceptor ─── TcpConnection ──...     │   accept + per-connection I/O
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                EventLoop                    │  the engine: loop + dispatch
│   poll() → for each ready ch: ch.handleback │   (owns a Poller)
└───────────────┬─────────────────────────────┘
                │ updatachannel / removechannel
┌───────────────▼─────────────────────────────┐
│                Poller  (abstract)           │
│                EpollPoller (epoll impl)     │   epoll_wait → active Channel*
└─────────────────────────────────────────────┘
                    │ fd ↔ Channel
┌─────────────────────────────────────────────┐
│   Channel  (event dispatch on one fd)       │
│   Socket   (RAII fd owner)                  │  base encap.
│   Buffer   (I/O cache, planned)             │
└─────────────────────────────────────────────┘
```

The design follows the classic **Reactor** pattern: a single thread owns the epoll fd and runs the loop; readiness is reported as `Channel*`, and callbacks are dispatched per event.

---

## Directory Layout

```
.
├── CMakeLists.txt
├── include/server/
│   ├── socket.hpp          # RAII fd wrapper
│   ├── noncopyable.hpp     # non-copyable base
│   ├── channel.hpp         # per-fd event dispatch
│   ├── poller.hpp          # Poller abstraction
│   ├── epollpoller.hpp     # epoll implementation
│   ├── event_loop.hpp/.cpp # the Reactor engine
│   ├── tcp_connnection.hpp # one managed connection
│   ├── acceptor.hpp        # (stub)
│   ├── thread_pool.hpp     # (stub)
│   ├── log.hpp             # Logger singleton
│   └── errno.hpp           # errno RAII guard
└── src/
    └── main.cpp            # entry point
```

---

## Build & Run

```bash
cmake -S . -B build
cmake --build build
./build/server          # currently a placeholder main loop
```

---

## Roadmap

- [x] Base wrappers: `Socket`, `Noncopyable`, `Logger`, `ErrnoGuard`
- [x] Reactor core: `Channel`, `Poller` / `EpollPoller`, `EventLoop`
- [ ] `Acceptor` — listen + accept, hand fds to `TcpConnection`
- [ ] `TcpConnection` I/O callbacks — single-connection echo over epoll
- [ ] `Buffer` — decouple plain `read`/`write` from socket readiness
- [ ] `ThreadPool` + multi-reactor loop threads
- [ ] Full echo server as the first end-to-end demo

---

## Notes for contributors

- Code style follows the *small file, clear layers* principle; header implementations are deliberately non-`inline` as part of the ongoing split into `.cpp`.
- The poller is abstract so the epoll backend can be swapped (e.g. `select`, `poll`) without touching `EventLoop`.
- Documentation and code comments are written to be readable by someone new to event-driven servers.

[WSL2]: https://learn.microsoft.com/en-us/windows/wsl/