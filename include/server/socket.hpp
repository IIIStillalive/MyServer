#pragma once
#include <unistd.h>                  // ::close（仅用于声明需要；实现已移 socket.cpp）


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

}