#include "tcpserver.hpp"

#include <algorithm>   // std::find_if

#include "server/log.hpp"

namespace server{

TcpServer::TcpServer(EventLoop* loop, const struct sockaddr_in& addr)
    : loop_(loop), acceptor_(loop, addr)
{
    // acceptor_ 构造时已 socket→bind→listen（监听 fd 非阻塞）；失败会在 start() 的 vaild() 拦下
}

bool TcpServer::start(){
    acceptor_.setNewConnectionCallback([this](int cfd){ this->onNewConnection(cfd); });
    if(!acceptor_.listen()) return false;   // 上树失败（含 socket/bind/listen 失败）
    return true;
}

void TcpServer::onNewConnection(int cfd){
    auto conn = std::make_unique<TcpConnection>(cfd, loop_);  // sock_(cfd) 接管已 accept 的 fd
    conn->setCloseCallback([this](TcpConnection* c){          // 连接关闭时移出容器并释放
        auto it = std::find_if(conns_.begin(), conns_.end(),
                    [c](const auto& u){ return u.get() == c; });
        if(it != conns_.end()) conns_.erase(it);  // erase → unique_ptr 析构 → sock_ close(fd)
    });
    if(!conn->listenconnection()){                // 内部 setreadback+enablereading+经 loop 上树
        Logger::instance().log(Level::ERROR, "TcpConnection::listenconnection failed, fd=", cfd);
        // make_unique 析构会关闭 cfd，无需手动 close，避免二次关闭
    } else {
        conns_.push_back(std::move(conn));
    }
}

}