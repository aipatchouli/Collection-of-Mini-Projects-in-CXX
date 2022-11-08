#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {
    struct timeval timeout = {2, 0}; // 2s的超时时间

    redisContext* pRedisContext = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if ((pRedisContext == nullptr) || ((pRedisContext->err) != 0)) {
        if (pRedisContext != nullptr) {
            std::cout << "connect error:" << &(pRedisContext->errstr) << std::endl;
        } else {
            std::cout << "connect error: can't allocate redis context." << std::endl;
        }
        return -1;
    }
    // redisReply是Redis命令回复对象 redis返回的信息保存在redisReply对象中
    auto* pRedisReply = static_cast<redisReply*>(redisCommand(pRedisContext, "INFO cpu")); // 执行INFO 命令，可以看到 redis 的一些详细情况
    std::cout << pRedisReply->str << std::endl;
    // 当多条Redis命令使用同一个redisReply对象时
    // 每一次执行完Redis命令后需要清空redisReply 以免对下一次的Redis操作造成影响
    freeReplyObject(pRedisReply);
    redisFree(pRedisContext);

    return 0;
}