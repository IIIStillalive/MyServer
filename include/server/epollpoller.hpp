#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "channel.hpp"
#include "poller.hpp"




namespace server{



class Epollpoller : public Poller{

public:
    Epollpoller() noexcept;
    ~Epollpoller() noexcept;

    bool updatechannel(Channel* ch);
    bool removechannel(Channel* ch);  //需要hannle提供是否remove,然后由epoller 执行DEL并erase;
    void poll(int timeoutMs, std::vector<Channel*>& active);  //epoll::wait()收集就绪的Channel

    bool isvaild() const;
private:
    int epfd_;
    std::vector<struct epoll_event> evs;
    std::unordered_map<int, Channel*> conns;  //这只是查找容器，并不持有资源,负责查找就绪返回的fd对应的存储的channel*

};

bool Epollpoller::isvaild() const{
    return epfd_ >= 0;
}

Epollpoller::Epollpoller() noexcept: epfd_(::epoll_create1(0)){} //create1() 失败了该怎么办?
Epollpoller::~Epollpoller(){
    if(isvaild())
        ::close(epfd_);
    epfd_= -1;
};

bool Epollpoller::updatechannel(Channel* ch){
    struct epoll_event ev;
    ev.data.fd = ch->fd();
    ev.events = ch->_events();
    auto it = conns.find(ev.data.fd);
    int op = (it == conns.end()) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD; //判断是否已经在连接中
    if(::epoll_ctl(epfd_, op, ch->fd(), &ev) < 0) return false;
    conns.emplace(ch->fd(), ch);
    return true;
}
bool Epollpoller::removechannel(Channel* ch){
    if(::epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), nullptr) < 0) return false;
    conns.erase(ch->fd());
    return true;
}

void Epollpoller::poll(int timeoutMs, std::vector<Channel*>& active){
    int n = ::epoll_wait(epfd_, evs.data(), 16, timeoutMs);
    if(n < 0) return;
    else if(n == 0) return;
    for(int i = 0; i < n; ++i){
        auto it = conns.find(evs[i].data.fd); //查找它的channel*，不直接构造。
        if(it == conns.end()) continue;
        it->second->setrevents(evs[i].events);
        active.emplace_back(it->second);//回传参数
    }
}

}