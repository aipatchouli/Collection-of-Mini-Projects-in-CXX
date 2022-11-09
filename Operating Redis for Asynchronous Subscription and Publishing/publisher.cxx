#include "redis_publisher.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    CXXRedisPublisher publisher;

    bool ret = publisher.init();
    if (!ret) {
        std::cout << "Init redis publisher failed." << std::endl;
        
        return 0;
    }

    ret = publisher.connect();
    if (!ret) {
        std::cout << "Connect redis failed." << std::endl;
        
        return 0;
    }

    while (true) {
        publisher.publish("test-channel", "Hello world!");
        usleep(1);
    }

    publisher.disconnect();
    publisher.uninit();
    return 0;
}