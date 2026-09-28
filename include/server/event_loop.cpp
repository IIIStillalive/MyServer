
#include "event_loop.hpp"
#include "epollpoller.hpp"
#include "channel.hpp"

namespace server{

EventLoop::EventLoop(): poller_(std::make_unique<Epollpoller>()) {}

// 析构放到 .cpp：这里 Epollpoller/Poller 已是完整类型，unique_ptr 能正确走虚析构释放
EventLoop::~EventLoop() = default;

bool EventLoop::updatachannel(Channel* ch){
    return poller_->updatechannel(ch);
}
void EventLoop::removechannel(Channel* ch){
    poller_->removechannel(ch);
}
void EventLoop::runloop(){
    while(true){
        poller_->poll(-1, readys);
        for(auto& it : readys){
            it->handleback();  //为就绪fd调用回调
        }
    }
}

}