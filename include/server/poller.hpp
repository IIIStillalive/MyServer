
#pragma once
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>


namespace server{
class Channel;

class Poller{
public:


    Poller() = default;
    virtual ~Poller() = default;  //虚析构函数，以便使用多态
    virtual void poll(int timeoutMs, std::vector<Channel*>& active) = 0;
    virtual bool updatechannel(Channel* ch) = 0;
    virtual bool removechannel(Channel* ch) = 0;
    
};



}