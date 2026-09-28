#include <cerrno>
#include <cstring>          // memset
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // htons/htonl

#include <memory>           // make_unique
#include <vector>           // 持有连接对象
#include <algorithm>        // std::find_if

#include "server/log.hpp"
#include "server/event_loop.hpp"
#include "server/tcp_connnection.hpp"
#include "server/acceptor.hpp"

using namespace server;

int main(){
    auto& log = Logger::instance();
    log.set_level(Level::TRACE);

    // 1) 绑定地址：只在 localhost:8888 上监听
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(8888);
    srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    EventLoop loop;

    // 2) 持有连接对象：必须活过 runloop()，由 close 回调释放（erase → unique_ptr 析构 → sock_ close(fd)）
    std::vector<std::unique_ptr<TcpConnection>> conns;

    // 3) 监听方封装：Acceptor 内部完成 socket→bind→listen→非阻塞，并注册监听 Channel
    Acceptor acceptor(&loop, srv);
    acceptor.setNewConnectionCallback([&loop, &conns, &log](int cfd){
        auto conn = std::make_unique<TcpConnection>(cfd, &loop);  // sock_(cfd) 接管已 accept 的 fd
        conn->setCloseCallback([&conns](TcpConnection* c){        // 连接关闭时通知这里释放
            auto it = std::find_if(conns.begin(), conns.end(),
                        [c](const auto& u){ return u.get() == c; });
            if(it != conns.end()) conns.erase(it);
        });
        if(!conn->listenconnection()){                 // 内部 setreadback+enablereading+经 loop 上树
            log.log(Level::ERROR, "TcpConnection::listenconnection failed, fd=", cfd);
            // make_unique 析构会关闭 cfd，无需手动 close，避免二次关闭
        } else {
            conns.push_back(std::move(conn));
        }
    });

    if(!acceptor.listen()){
        log.log(Level::ERROR, "Acceptor::listen failed, exit");
        return -1;
    }

    log.log(Level::INFO, "listening on 127.0.0.1:8888, runloop starts");
    loop.runloop();       // 阻塞事件循环，不再返回

    return 0;
}