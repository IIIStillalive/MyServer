#include "tcpserver.hpp"

#include <functional>         // std::bind
#include "server/log.hpp"
#include "server/event_loop.hpp"  // EventLoop 完整定义：runInLoop/runloop 需完整类型

namespace server{

TcpServer::TcpServer(EventLoop* loop, const struct sockaddr_in& addr)
    : loop_(loop), acceptor_(loop, addr), threadPool_(loop)
{
    // acceptor_ 构造时已 socket→bind→listen；失败在 start() 的 vaild() 拦下
    // threadPool_(loop) 传 main loop 作降级目标（线程数 0 时 getNextLoop 返回它）
}

void TcpServer::setThreadNum(size_t n){
    threadPool_.setThreadNum(n);
}
void TcpServer::setMessageCallback(const MessageCallback& cb){
    codec_.setMessageCallback(cb);   // 业务层：收到一条完整消息时被调用
}
void TcpServer::sendMessage(TcpConnection* conn, const void* data, size_t n){
    codec_.send(conn, data, n);      // 编码 [长度头][payload] 再交给底层传输（业务回发必经此口）
}

bool TcpServer::start(){
    threadPool_.start();                                  // ① 先起 worker（连接随时可能来）
    acceptor_.setNewConnectionCallback([this](int cfd){ this->onNewConnection(cfd); });
    if(!acceptor_.listen()) return false;                 // ② 再开监听
    return true;
}

void TcpServer::onNewConnection(int cfd){                 // 在 main loop 线程
    EventLoop* ioLoop = threadPool_.getNextLoop();        // 轮询挑一个 worker loop
    ioLoop->runInLoop([this, ioLoop, cfd]{                // 派到 ioLoop 线程执行
        // —— 以下全部在 ioLoop 线程 ——
        auto conn = std::make_shared<TcpConnection>(cfd, ioLoop);  // 连接归 ioLoop；shared_ptr 供回调栈保活
        conn->setCloseCallback([this](TcpConnection* c){
            // 关闭回调：拉回 main loop 线程移除映射（conns_ 只许 main 线程碰）
            // ⚠️ 绝不能捕获 conn(shared_ptr)：TcpConnection 成员持有捕获自己 shared_ptr 的闭包 → 循环引用，
            //    使对象永不析构、fd 永不 close → 连接不关 + fd 泄漏。
            // 只在回调栈内(对象仍活着)同步取 fd(int)，之后 runInLoop 不依赖对象存活。
            int cfd = c->fd();
            loop_->runInLoop([this, cfd]{ removeConnection(cfd); });
        });
        // 解析层挂钩：读到字节 → Codec 剥长度头切消息 → 业务 messageCallback
        conn->setMessageCallback([this](TcpConnection* conn, Buffer* buf) {
            codec_.onMessage(conn, buf);
        });
        if(conn->listenconnection()){                     // 上树发生在 ioLoop 的 epoll
            // 登记也拉回 main：conns_ 的插入/删除全走 main runInLoop 串行 → 无跨线程 race
            // insert 在 listenconnection 成功后立刻提交、close 只在其后同线程触发 → FIFO 保证先记后删
            loop_->runInLoop([this, cfd, conn]{
                conns_[cfd] = std::move(conn);            // 单一映射，key=fd；main 线程
            });
        }
        // listenconnection 失败：shared_ptr 析构自动 close(fd)，无需手动
        Logger::instance().log(Level::INFO, "threadID: ",std::this_thread::get_id());
    });
}

void TcpServer::removeConnection(int cfd){                // 在 main loop 线程
    conns_.erase(cfd);                                    // 丢 shared_ptr → 若再无其他引用则析构 → close(fd)
}

}