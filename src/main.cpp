#include <cstring>          // memset
#include <string>
#include <csignal>          // sigaction
#include <atomic>
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // htons/htonl

#include "server/log.hpp"
#include "server/event_loop.hpp"
#include "server/tcpserver.hpp"

using namespace server;

namespace {                       // 匿名命名空间：全局可达，供 signal handler 使用
    std::atomic<server::EventLoop*> g_loop{nullptr};

    void handleSig(int){          // async-signal-safe：只调 quit()（atomic store + eventfd write，无锁无日志）
        auto* lp = g_loop.load(std::memory_order_relaxed);
        if(lp) lp->quit();
    }
}

int main(){
    auto& log = Logger::instance();
    log.set_level(Level::TRACE);

    // 1) 绑定地址：只在 localhost:8888 上监听
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(8888);
    srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // 2) 服务器门面：Acceptor（监听）+ 连接持有 + ThreadPool 多线程都封装在 TcpServer 内
    EventLoop loop;
    TcpServer server(&loop, srv);
    server.setThreadNum(4);        // 4 个 worker；设 0 则单线程降级
    // 业务层：收到"一条完整消息"（长度前缀已由 Codec 剥掉）后决定怎么应答。
    // 样本：回显。真实业务在这里替换成转发/处理/落库。
    server.setMessageCallback([&server](TcpConnection* conn, const std::string& msg){
        server.sendMessage(conn, msg.data(), msg.size());  // 必须经 Codec 编长度头；不能裸 conn->send
    });
    if(!server.start()){
        log.log(Level::ERROR, "TcpServer::start failed, exit");
        return -1;
    }

    g_loop.store(&loop);          // 先存指针，再装 handler（避免 handler 在 store 前被触发）
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSig;
    sigaction(SIGINT,  &sa, nullptr);   // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr);

    log.log(Level::INFO, "listening on 127.0.0.1:8888, runloop starts");
    loop.runloop();       // 阻塞事件循环；被 SIGINT 打断后优雅退出

    return 0;
}