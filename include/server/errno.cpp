#include "server/errno.hpp"

namespace server{

ErrnoGuard::ErrnoGuard() noexcept : saved_{errno}, prev_{errno} {}
ErrnoGuard::~ErrnoGuard() noexcept{
    errno = prev_;   // errno 是宏,展开为 lvalue,去掉 ::
}
int ErrnoGuard::code() const noexcept{
    return this->saved_;
}
std::string ErrnoGuard::message() const{
    return std::strerror(saved_);
}
bool ErrnoGuard::ok() const noexcept{
    return saved_ == 0;
}

}