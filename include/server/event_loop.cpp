
#include "event_loop.hpp"
#include "epollpoller.hpp"
#include "channel.hpp"
#include "timer_queue.hpp"
#include "log.hpp"
#include "errno.hpp"

namespace server{

EventLoop::EventLoop():poller_(std::make_unique<Epollpoller>()), timerQueue_(new TimerQueue(this)), thread_id_(std::this_thread::get_id()), eventsoc_(createEventfd()), eventch_(std::make_unique<Channel>(eventsoc_.fd(), this)){
    eventch_->setreadback([this]{ handlewake(); });
    eventch_->enablereading();
}

// 析构放到 .cpp：这里 Epollpoller/Poller 已是完整类型，unique_ptr 能正确走虚析构释放
EventLoop::~EventLoop(){
    quit();
}
void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()) wakeup();
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb){
    int64_t expiry = nowMicros() + static_cast<int64_t>(delay * 1000000.0);
    return timerQueue_->addTimer(std::move(cb), expiry, 0);
}
TimerId EventLoop::runEvery(double interval, TimerCallback cb){
    int64_t iv = static_cast<int64_t>(interval * 1000000.0);
    return timerQueue_->addTimer(std::move(cb), nowMicros() + iv, iv);
}
void EventLoop::cancel(TimerId id){
    timerQueue_->cancel(id);
}
bool EventLoop::updatachannel(Channel* ch){
    return poller_->updatechannel(ch);
}
void EventLoop::removechannel(Channel* ch){
    poller_->removechannel(ch);
}
void EventLoop::runloop(){
    while(!quit_){
        poller_->poll(-1, readys);
        for(auto& it : readys){
            it->handleback();  //为就绪fd调用回调
        }
        //dopendingFunctors();  //额外执行一次,多余
    }
}
int EventLoop::createEventfd(){
    int eventfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(eventfd < 0){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "createEventfd() failed ", eg.message());
        return -1;
    }
    return eventfd;
}
bool EventLoop::isInLoopThread() const{
    return (std::this_thread::get_id() == this->thread_id_);
}

bool EventLoop::wakeup(){
    uint64_t one = 1;
    size_t n = ::write(eventsoc_.fd(), &one, sizeof(one));
    if(n != sizeof(one)){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "wakeup() failed", eg.message());
        return false;
    }
    return true;
}

void EventLoop::runInLoop(Callback cb){
    if(isInLoopThread()){
        cb();  //在当前线程就执行
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);  //跨线程加锁
        pendingFunctors_.emplace_back(cb);  //加入任务队列,一般这些任务是调用poller进行注册
    }
    wakeup();  //唤醒目标线程
    
}

bool EventLoop::handlewake(){  
    //处理唤醒就和main中处理lfd的处理一致，它主要就是呼应write并对工作队列进行处理
    //就是这个子线程lfd的回调函数
    uint64_t one = 1;
    int n = ::read(eventsoc_.fd(), &one, sizeof(one));
    if(n != sizeof(one) && errno != EAGAIN){
        ErrnoGuard eg;
        Logger::instance().log(Level::ERROR, "handlewake() failed", eg.message());
        return false;
    }
    dopendingFunctors();  //处理任务队列，主要就是循环清空就可以了
    return true;
}

void EventLoop::dopendingFunctors(){
    std::vector<Callback> functors;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 用 swap 减少持锁时间
    }
    for(auto& cb : functors){
        cb();  //进行任务队列的处理 
    }
}
}