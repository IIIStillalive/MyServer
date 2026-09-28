#include "server/log.hpp"

#include <iostream>

namespace server{

const char* level_str(Level lv){
    switch (lv) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}

Logger& Logger::instance(){
    static Logger inst;
    return inst;
}

void Logger::do_log(Level lv, const std::string& msg){
    std::cout << msg << '\n';
    (void)lv;
}

void Logger::set_level(Level lv){
    this->level_ = lv;
}

}