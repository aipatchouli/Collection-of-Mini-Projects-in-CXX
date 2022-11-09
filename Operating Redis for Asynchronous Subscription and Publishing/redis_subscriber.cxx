#include "redis_subscriber.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <thread>

CXXRedisSubscriber::CXXRedisSubscriber()
    : _event_base(nullptr), _event_thread(0),
      _redis_context(nullptr) {
}

bool CXXRedisSubscriber::init(const NotifyMessageFn& fn) {
    _notify_message_fn = fn;
    _event_base = event_base_new(); // 创建libevent对象
    if (nullptr == _event_base) {
        std::cout << "Create radis event failed." << std::endl;
        return false;
    }

    memset(&_event_sem, 0, sizeof(_event_sem));
    int ret = sem_init(&_event_sem, 0, 0);
    if (ret != 0) {
        std::cout << "Create redis event sem failed." << std::endl;
        return false;
    }

    return true;
}

bool CXXRedisSubscriber::uninit() {
    _event_base = nullptr;
    sem_destroy(&_event_sem);
    return true;
}

bool CXXRedisSubscriber::connect() {
    // connect redis
    _redis_context = redisAsyncConnect("127.0.0.1", 6379); // 异步连接到redis服务器上，使用默认端口
    if (nullptr == _redis_context) {
        std::cout << "Connect redis failed." << std::endl;
        return false;
    }

    if (_redis_context->err != 0) {
        std::cout << "Connect redis failed, error: " << _redis_context->err << _redis_context->errstr << std::endl;
        return false;
    }

    // attach the event
    redisLibeventAttach(_redis_context, _event_base); // 将事件绑定到redis context上，使设置给redis的回调跟事件关联

    // 创建事件处理线程
    /* int ret = pthread_create(&_event_thread, nullptr, &CXXRedisSubscriber::event_thread, this);
    if (ret != 0) {
        printf(": create event thread failed.\n");
        disconnect();
        return false;
    } */
    auto thread = std::thread(&CXXRedisSubscriber::event_proc, this);
    thread.detach();

    // 设置连接回调，当异步调用连接后，服务器处理连接请求结束后调用，通知调用者连接的状态
    redisAsyncSetConnectCallback(_redis_context, &CXXRedisSubscriber::connect_callback);

    // 设置断开连接回调，当服务器断开连接后，通知调用者连接断开，调用者可以利用这个函数实现重连
    redisAsyncSetDisconnectCallback(_redis_context, &CXXRedisSubscriber::disconnect_callback);

    // 启动事件线程
    sem_post(&_event_sem);
    return true;
}

bool CXXRedisSubscriber::disconnect() {
    if (_redis_context != nullptr) {
        redisAsyncDisconnect(_redis_context);
        redisAsyncFree(_redis_context);
        _redis_context = nullptr;
    }

    return true;
}

bool CXXRedisSubscriber::subscribe(const std::string& channel_name) {
    int ret = redisAsyncCommand(_redis_context, &CXXRedisSubscriber::command_callback, this, "SUBSCRIBE %s", channel_name.c_str());
    if (REDIS_ERR == ret) {
        std::cout << "Subscribe channel failed." << std::endl;
        return false;
    }
    std::cout << "Subscribe channel success: " << channel_name << std::endl;

    return true;
}

void CXXRedisSubscriber::connect_callback(const redisAsyncContext* redis_context,
                                          int status) {
    if (status != REDIS_OK) {
        std::cout << "Connect redis failed, error: " << redis_context->err << redis_context->errstr << std::endl;
    } else {
        std::cout << "Connect redis success." << std::endl;
    }
}

void CXXRedisSubscriber::disconnect_callback(
    const redisAsyncContext* redis_context, int status) {
    if (status != REDIS_OK) {
        // 这里异常退出，可以尝试重连
        std::cout << "Disconnect redis failed, error: " << redis_context->err << redis_context->errstr << std::endl;
    }
}

// 消息接收回调函数
void CXXRedisSubscriber::command_callback(redisAsyncContext* /*redis_context*/,
                                          void* reply, void* privdata) {
    if (nullptr == reply || nullptr == privdata) {
        return;
    }

    // 静态函数中，要使用类的成员变量，把当前的this指针传进来，用this指针间接访问
    auto* self_this = (CXXRedisSubscriber*)(privdata);
    auto* redis_reply = (redisReply*)(reply);

    // 订阅接收到的消息是一个带三元素的数组
    if (redis_reply->type == REDIS_REPLY_ARRAY && redis_reply->elements == 3) {
        std::cout << "Receive message: " << redis_reply->element[0]->str << redis_reply->element[0] << std::endl;
        std::cout << "Receive message: " << redis_reply->element[1]->str << redis_reply->element[1] << std::endl;
        std::cout << "Receive message: " << redis_reply->element[2]->str << redis_reply->element[2] << std::endl;

        // 调用函数对象把消息通知给外层
        self_this->_notify_message_fn(redis_reply->element[1]->str,
                                      redis_reply->element[2]->str, redis_reply->element[2]->len);
    }
}

void* CXXRedisSubscriber::event_thread(void* data) {
    if (nullptr == data) {
        std::cout << "Event thread data is null." << std::endl;
        return nullptr;
    }

    CXXRedisSubscriber* self_this = reinterpret_cast<CXXRedisSubscriber*>(data);
    return self_this->event_proc();
}

void* CXXRedisSubscriber::event_proc() {
    sem_wait(&_event_sem);

    // 开启事件分发，event_base_dispatch会阻塞
    event_base_dispatch(_event_base);

    return nullptr;
}