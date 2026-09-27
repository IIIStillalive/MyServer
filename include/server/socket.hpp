#pragma once
#include <unistd.h>                  // ::close


namespace server{

class Socket{

public:
    explicit Socket(int fd = -1) noexcept;
    ~Socket() noexcept;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int fd() const noexcept;
    bool vaild() const noexcept;
    void close() noexcept;

private:

    int fd_;
};

Socket::Socket(int fd) noexcept: fd_(fd) {}
Socket::~Socket()noexcept{
    close();
}
int Socket::fd()const noexcept{
    return fd_;
}
bool Socket::vaild()const noexcept{
    return fd_ >= 0;
}
void Socket::close() noexcept{
    if(vaild()) ::close(fd_);
    return;
}
}


