#include "server/event_loop.hpp"

using namespace server;

int main(){
    EventLoop loop;
    loop.runloop();   // 先空转，验证 reactor 骨架能跑
    return 0;
}
