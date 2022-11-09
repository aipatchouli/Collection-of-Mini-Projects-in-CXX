#include "redis_subscriber.hpp"
#include <iostream>

void recieve_message(const char* channel_name, const char* message, int len) {
    std::cout << "Recieve Message:" << '\n';
    std::cout << "Channel Name:" << channel_name << '\n';
    std::cout << "Message:" << message << std::endl;
}

int main(int  /*argc*/, char*  /*argv*/[]) {
    CXXRedisSubscriber subscriber;
    CXXRedisSubscriber::NotifyMessageFn fn = bind(recieve_message, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3);

    bool ret = subscriber.init(fn);
    if (!ret) {
        std::cout << "Init Failed" << std::endl;
        return 0;
    }

    ret = subscriber.connect();
    if (!ret) {
        std::cout << "Connect Failed" << std::endl;
        return 0;
    }

    subscriber.subscribe("test-channel");

    while (true) {
        usleep(1);
    }

    subscriber.disconnect();
    subscriber.uninit();

    return 0;
}