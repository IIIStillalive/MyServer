#pragma once

#include <sstream>
#include <utility>
#include <string>
#include "server/noncopyable.hpp"
namespace server
{

enum class Level{ TRACE, DEBUG, INFO, WARN, ERROR};

// 一对自由函数: 等级->字符串（实现已移 log.cpp）
const char* level_str(Level lv);

class Logger: public Noncopyable
{
public:
    static Logger& instance();
    void set_level(Level lv);
    template<typename... Args>
    void log(Level lv, Args&&... args)
    {
        std::ostringstream os;
        os << level_str(lv) << ":";
        (os << ... << std::forward<Args>(args));
        do_log(lv, os.str());

    }
private:
    Logger() = default; //私有只能由instance 创建
    void do_log(Level lv, const std::string& msg);
    Level level_ = Level::INFO;
};

}