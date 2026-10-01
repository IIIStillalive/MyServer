#include <cstring>          // memset
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // htons/htonl

#include "server/log.hpp"
#include "server/event_loop.hpp"
#include "server/tcpserver.hpp"

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

    // 2) 服务器门面：Acceptor（监听）+ 全部连接的持有/创建/释放都封装在 TcpServer 内
    EventLoop loop;
    TcpServer server(&loop, srv);
    if(!server.start()){
        log.log(Level::ERROR, "TcpServer::start failed, exit");
        return -1;
    }

    log.log(Level::INFO, "listening on 127.0.0.1:8888, runloop starts");
    loop.runloop();       // 阻塞事件循环，不再返回

    return 0;
}