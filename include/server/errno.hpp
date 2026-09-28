#pragma once

#include <cerrno>
#include <cstring>
#include <string>


namespace server
{
class ErrnoGuard
{
public:
    explicit ErrnoGuard() noexcept;
    ~ErrnoGuard() noexcept;

    int code() const noexcept;
    std::string message() const;
    bool ok() const noexcept;

    ErrnoGuard(const ErrnoGuard&) = delete;
    ErrnoGuard& operator=(const ErrnoGuard& ) = delete;
private:
    int saved_;
    int prev_;
};

}