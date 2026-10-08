// TimerQueue 独立演示：单次 runAfter + 循环 runEvery + cancel，跑完自动 quit
// 期望日志顺序：
//   every 0.5s tick #1..#3
//   once after 1.6s -> cancel every-timer
//   (3.0s) 3.0s up, quit   ← 循环已停，故 2.0s 后不再有 tick
#include <string>
#include <memory>
#include "server/event_loop.hpp"
#include "server/log.hpp"

using namespace server;

int main(){
    auto& log = Logger::instance();
    log.set_level(Level::INFO);

    EventLoop loop;

    // 每 0.5s 循环打一条（存储 id，供后续 cancel）
    TimerId tickId = loop.runEvery(0.5, [&log, cnt = std::make_shared<int>(0)](){
        (*cnt)++;
        log.log(Level::INFO, "every 0.5s tick #", std::to_string(*cnt));
    });

    // 1.6s 后执行一次：取消上面的循环，验证 cancel
    loop.runAfter(1.6, [&log, &loop, tickId]{
        log.log(Level::INFO, "once after 1.6s -> cancel every-timer");
        loop.cancel(tickId);
    });

    // 3.0s 退出
    loop.runAfter(3.0, [&log, &loop]{
        log.log(Level::INFO, "3.0s up, quit");
        loop.quit();
    });

    loop.runloop();
    return 0;
}