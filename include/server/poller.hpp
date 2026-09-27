
#pragma once
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>


namespace server{
class Channel;

class Poller{
public:


    Poller() = default;
    ~Poller() = default;
    virtual void poll(int timeoutMs, std::vector<Channel*>& active) = 0;
    virtual bool updatechannel(Channel* ch) = 0;
    virtual bool removechannel(Channel* ch) = 0;

};



}