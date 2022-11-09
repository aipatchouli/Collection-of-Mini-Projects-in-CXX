#ifndef REDIS_SUBSCRIBER_HPP
#define REDIS_SUBSCRIBER_HPP

#include <boost/tr1/functional.hpp>
#include <cstdlib>
#include <hiredis/adapters/libevent.h>
#include <hiredis/async.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <unistd.h>
#include <vector>

class CXXRedisSubscriber {
public:
    using NotifyMessageFn = std::tr1::function<void(const char*, const char*, int)>;

    CXXRedisSubscriber();
    ~CXXRedisSubscriber() = default;

    bool init(const NotifyMessageFn& fn); // 传入回调对象
    bool uninit();
    bool connect();
    bool disconnect();

    // 可以多次调用，订阅多个频道
    bool subscribe(const std::string& channel_name);

private:
    // 下面三个回调函数供redis服务调用
    // 连接回调
    static void connect_callback(const redisAsyncContext* redis_context, int status);

    // 断开连接的回调
    static void disconnect_callback(const redisAsyncContext* redis_context, int status);

    // 执行命令回调
    static void command_callback(redisAsyncContext* redis_context, void* reply, void* privdata);

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

    // 通知外层的回调函数对象
    NotifyMessageFn _notify_message_fn;
};

#endif