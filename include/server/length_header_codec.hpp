#pragma once

#include <functional>
#include <string>
#include <cstddef>

namespace server{

class TcpConnection;
class Buffer;

// 长度前缀编解码（Muduo Style）：[4 字节大端长度][payload]。
//   解决粘包/半包：onMessage 每次从 Buffer 里切出一条完整消息，交给业务回调；
//   半包攒在 Buffer 里等下一段凑齐，粘包则本循环逐条切。
class LengthHeaderCodec{
public:
    using MessageCallback = std::function<void(TcpConnection*, const std::string&)>;

    void setMessageCallback(const MessageCallback& cb){ messageCallback_ = cb; }

    void onMessage(TcpConnection* conn, Buffer* buf);  // 剥长度头 → 一条完整消息 → 业务回调
    void send(TcpConnection* conn, const void* data, size_t n);  // 编码长度前缀后发送

private:
    MessageCallback messageCallback_;   // 业务层：收到一条完整消息
};

}