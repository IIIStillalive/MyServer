#include "buffer.hpp"
#include <sys/uio.h>   // readv / struct iovec
#include <cstring>     // memcpy
#include <cerrno>      // errno
#include <sys/errno.h>

namespace server{

void Buffer::makeSpace(size_t n){
    if(writeableBytes() >= n) return;            // 尾部够，直接写
    //下面是不够
    //不够看以前读完的空间和没写的空间加起来够不够
    
    size_t readable = readableBytes();
    if(writeableBytes() + readerIndex_ >= n){    // 整体搬移到头部后就够 → 搬移
        std::memmove(&buffer_[0], &buffer_[readerIndex_], readable);
        readerIndex_ = 0;
        writerIndex_ = readable;
    } else {                                      // 不够 → 扩容
        buffer_.resize(writerIndex_ + n);
    }
}

void Buffer::append(const char* data, size_t n){
    makeSpace(n);
    std::memcpy(&buffer_[writerIndex_], data, n);
    writerIndex_ += n;
}

void Buffer::retrieve(size_t n){
    if(n >= readableBytes()){ retrieveAll(); return; }
    readerIndex_ += n;
}

void Buffer::retrieveAll(){
    readerIndex_ = 0;
    writerIndex_ = 0;
}

ssize_t Buffer::readFd(int fd, int* saveErrno){
    char extrabuf[65536];
    struct iovec vec[2];
    size_t writable = writeableBytes();

    vec[0].iov_base = &buffer_[writerIndex_];
    vec[0].iov_len  = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len  = sizeof(extrabuf);

    // 尾部能写满 extrabuf 就不用第二段，减少一次拷贝
    ssize_t n = ::readv(fd, vec, writable < sizeof(extrabuf) ? 2 : 1);
    if(n < 0){
        *saveErrno = errno;
    } else if(static_cast<size_t>(n) <= writable){
        writerIndex_ += n;                       // 全落在 buffer_
    } else {
        writerIndex_ = buffer_.size();           // 尾部写满
        append(extrabuf, n - writable);          // 溢出部分进 buffer
    }
    return n;
}

}
