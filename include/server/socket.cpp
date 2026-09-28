#include "socket.hpp"

#include <unistd.h>   // ::close

namespace server{

Socket::Socket(int fd) noexcept: fd_(fd) {}
Socket::~Socket() noexcept{
    close();
}
int Socket::fd() const noexcept{
    return fd_;
}
bool Socket::vaild() const noexcept{
    return fd_ >= 0;
}
void Socket::close() noexcept{
    if(vaild()) ::close(fd_);
}

}