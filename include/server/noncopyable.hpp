#pragma once

namespace server{

// 继承它则禁止拷贝/赋值。CRTP不是必须，空基类即可。
class Noncopyable{
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}