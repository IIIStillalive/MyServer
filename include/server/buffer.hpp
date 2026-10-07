#pragma once
#include <vector>
#include <sys/types.h>   // ssize_t

namespace server{

// 应用层缓冲区：解决 readFd 读入 + write 侧累积，配合可写事件补发。
// 下标模型：buffer_[readerIndex_, writerIndex_) 是未读数据。
class Buffer{
public:
    Buffer() = default;
    ~Buffer() = default;

    size_t readableBytes() const { return writerIndex_ - readerIndex_; } // 未读
    size_t writeableBytes() const { return buffer_.size() - writerIndex_; } // 可写
    const char* peek() const { return &buffer_[readerIndex_]; }          // 读指针

    void retrieve(size_t n);            // 消费前面 n 字节（读过了）
    void retrieveAll();                 // 消费全部
    void append(const char* data, size_t n);  // 尾部追加 n 字节

    ssize_t readFd(int fd, int* saveErrno);  // 从 fd 读进缓冲（readv 减拷贝）

private:
    void makeSpace(size_t n);           // 保证尾部还能写 n 字节（必要时搬移 + 扩容）
    std::vector<char> buffer_;
    size_t readerIndex_ = 0;
    size_t writerIndex_ = 0;
};

}
