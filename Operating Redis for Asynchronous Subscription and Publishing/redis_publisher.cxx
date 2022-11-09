#include "redis_publisher.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <thread>

CXXRedisPublisher::CXXRedisPublisher()
    : _event_base(nullptr), _event_thread(0),
      _redis_context(nullptr) {
}

bool CXXRedisPublisher::init() {
    _event_base = event_base_new(); // 创建libevent对象
    if (_event_base == nullptr) {
        std::cout << ": Create redis event failed" << std::endl;
        return false;
    }
    memset(&_event_sem, 0, sizeof(_event_sem)); // 初始化信号量
    int ret = sem_init(&_event_sem, 0, 0);
    if (ret != 0) {
        std::cout << ": Create redis event semaphore failed" << std::endl;
        return false;
    }

    return true;
}

bool CXXRedisPublisher::uninit() {
    _event_base = nullptr;
    sem_destroy(&_event_sem);
    return true;
}

bool CXXRedisPublisher::connect() {
    _redis_context = redisAsyncConnect("127.0.0.1", 6379); // 异步连接到redis服务器上，使用默认端口
    if (_redis_context == nullptr) {
        std::cout << ": Create redis async context failed" << std::endl;
        return false;
    }

    if (_redis_context->err != 0) {
        std::cout << ": Connect to redis server failed: " << _redis_context->errstr << ", " << _redis_context->errstr << std::endl; // 输出错误信息
        return false;
    }

    redisLibeventAttach(_redis_context, _event_base); // 将事件绑定到redis context上，使设置给redis的回调跟事件关联

    // 创建事件处理线程
    /* int ret = pthread_create(&_event_thread, nullptr, &CXXRedisPublisher::event_thread, this);
    if (ret != 0) {
        std::cout << ": Create redis event thread failed" << std::endl;
        disconnect();
        return false;
    } */
    auto thread = std::thread(&CXXRedisPublisher::event_proc, this);
    thread.detach();

    // 设置连接回调，当异步调用连接后，服务器处理连接请求结束后调用，通知调用者连接的状态
    redisAsyncSetConnectCallback(_redis_context, &CXXRedisPublisher::connect_callback);

    // 设置断开连接回调，当服务器断开连接后，通知调用者连接断开，调用者可以利用这个函数实现重连
    redisAsyncSetDisconnectCallback(_redis_context, &CXXRedisPublisher::disconnect_callback);

    // 启动事件线程
    sem_post(&_event_sem);
    return true;
}

bool CXXRedisPublisher::disconnect() {
    if (_redis_context != nullptr) {
        redisAsyncDisconnect(_redis_context);
        redisAsyncFree(_redis_context);
        _redis_context = nullptr;
    }

    return true;
}

bool CXXRedisPublisher::publish(const std::string& channel_name, const std::string& message) {
    int ret = redisAsyncCommand(_redis_context, &CXXRedisPublisher::command_callback, this, "PUBLISH %s %s", channel_name.c_str(), message.c_str());
    if (REDIS_ERR == ret) {
        std::cout << ": Publish message failed: " << ret << std::endl;
        return false;
    }

    return true;
}

void CXXRedisPublisher::connect_callback(const redisAsyncContext* redis_context,
                                         int status) {
    if (status != REDIS_OK) {
        std::cout << ": Connect to redis server failed: " << redis_context->errstr << ", " << redis_context->err << std::endl;
    } else {
        std::cout << ": Connect to redis server success" << std::endl;
    }
}

void CXXRedisPublisher::disconnect_callback(
    const redisAsyncContext* redis_context, int status) {
    if (status != REDIS_OK) {
        // 这里异常退出，可以尝试重连
        std::cout << ": Disconnect to redis server failed: " << redis_context->errstr << ", " << redis_context->err << std::endl;
    }
}

// 消息接收回调函数
void CXXRedisPublisher::command_callback(redisAsyncContext* redis_context, void* reply, void* privdata) {
    std::cout << ": Command callback success" << std::endl;
    // 这里不执行任何操作
}

void* CXXRedisPublisher::event_thread(void* data) {
    if (data == nullptr) {
        std::cout << ": Event thread data is null" << std::endl;
        return nullptr;
    }

    auto* self_this = (CXXRedisPublisher*)(data);
    return self_this->event_proc();
}

void* CXXRedisPublisher::event_proc() {
    sem_wait(&_event_sem);

    // 开启事件分发，event_base_dispatch会阻塞
    event_base_dispatch(_event_base);
    return nullptr;
}