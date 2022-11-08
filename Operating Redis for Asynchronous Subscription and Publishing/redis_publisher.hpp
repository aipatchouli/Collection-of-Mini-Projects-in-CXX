#ifndef REDIS_PUBLISHER_HPP
#define REDIS_PUBLISHER_HPP

#include <boost/tr1/functional.hpp>
#include <cstdlib>
#include <hiredis/adapters/libevent.h>
#include <hiredis/async.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <unistd.h>
#include <vector>

class CXXRedisPublisher {
public:
    CXXRedisPublisher();
    ~CXXRedisPublisher() = default;

    CXXRedisPublisher(const CXXRedisPublisher&) = default;
    CXXRedisPublisher(CXXRedisPublisher&&) = delete;
    CXXRedisPublisher& operator=(const CXXRedisPublisher&) = default;
    CXXRedisPublisher& operator=(CXXRedisPublisher&&) = delete;
    

    bool init();
    bool uninit();
    bool connect();
    bool disconnect();

    bool publish(const std::string& channel_name,
                 const std::string& message);

private:
    // 下面三个回调函数供redis服务调用
    // 连接回调
    static void connect_callback(const redisAsyncContext* redis_context,
                                 int status);

    // 断开连接的回调
    static void disconnect_callback(const redisAsyncContext* redis_context,
                                    int status);

    // 执行命令回调
    static void command_callback(redisAsyncContext* redis_context,
                                 void* reply, void* privdata);

    // 事件分发线程函数
    static void* event_thread(void* data);
    void* event_proc();


    // libevent事件对象
    event_base* _event_base;
    // 事件线程ID
    pthread_t _event_thread;
    // 事件线程的信号量
    sem_t _event_sem;
    // hiredis异步对象
    redisAsyncContext* _redis_context;
};

#endif // REDIS_PUBLISHER_HPP