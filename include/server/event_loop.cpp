
#include "event_loop.hpp"
#include "epollpoller.hpp"
#include "channel.hpp"

namespace server{

EventLoop::EventLoop(): poller_(new Epollpoller()) {}

void EventLoop::updatachannel(Channel* ch){
    poller_->updatechannel(ch);
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