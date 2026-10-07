#include "length_header_codec.hpp"

#include <cstdint>

#include "server/buffer.hpp"
#include "server/tcp_connnection.hpp"   // TcpConnection::send 需完整类型
#include "server/log.hpp"

namespace server{

namespace {
const int kHeaderLen = 4;      // 长度头的字节数
const int kMaxMsgLen = 65536;  // 单条消息上限，防非法长度把 buffer 撑爆

// 大端 4 字节 → int32_t
int32_t readInt32BigEndian(const char* p){
    const auto* u = reinterpret_cast<const unsigned char*>(p);
    return (static_cast<int32_t>(u[0]) << 24) | (static_cast<int32_t>(u[1]) << 16)
         | (static_cast<int32_t>(u[2]) << 8)  |  static_cast<int32_t>(u[3]);
}
}

void LengthHeaderCodec::onMessage(TcpConnection* conn, Buffer* buf){
    while(buf->readableBytes() >= kHeaderLen){
        int32_t len = readInt32BigEndian(buf->peek());
        if(len < 0 || len > kMaxMsgLen){     // 对端发来非法长度 → 协议错，本层记日志丢弃
            Logger::instance().log(Level::ERROR, "invalid message length: ", len, ", discard");
            return;
        }
        if(buf->readableBytes() < static_cast<size_t>(kHeaderLen + len))
            break;                            // 半包：凑不足一条，留 buffer 等下一段

        std::string msg(buf->peek() + kHeaderLen, len);  // 本条 payload（拷贝）
        buf->retrieve(kHeaderLen + len);      // 消费：长度头 + payload

        if(messageCallback_) messageCallback_(conn, msg);
    }
}

void LengthHeaderCodec::send(TcpConnection* conn, const void* data, size_t n){
    std::string msg;
    msg.reserve(kHeaderLen + n);

    uint32_t be = static_cast<uint32_t>(n);    // 大端长度头
    char lenbuf[kHeaderLen];
    lenbuf[0] = static_cast<char>((be >> 24) & 0xFF);
    lenbuf[1] = static_cast<char>((be >> 16) & 0xFF);
    lenbuf[2] = static_cast<char>((be >> 8)  & 0xFF);
    lenbuf[3] = static_cast<char>(be         & 0xFF);
    msg.append(lenbuf, kHeaderLen);
    msg.append(static_cast<const char*>(data), n);

    conn->send(msg.data(), msg.size());       // 底层传输侧（Buffer + EPOLLOUT 补发）
}

}