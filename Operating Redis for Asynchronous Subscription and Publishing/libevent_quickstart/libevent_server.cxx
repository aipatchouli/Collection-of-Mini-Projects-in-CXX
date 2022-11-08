/**
You need libevent2 to compile this piece of code
Please see: http://libevent.org/
Or you can simply run this command to install on Mac: brew install libevent
Cmd to compile this piece of code: g++ LibeventQuickStartServer.c  -o  LibeventQuickStartServer /usr/local/lib/libevent.a
**/
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <event.h>
#include <iostream>
#include <unistd.h>

void accept_cb(int fd, int16_t events, void* arg);
void socket_read_cb(int fd, int16_t events, void* arg);

int tcp_server_init(int port, int listen_num);

int main(int /*argc*/, char const* /*argv*/[]) {
    /* code */
    int listener = tcp_server_init(9999, 10);
    if (listener == -1) {
        perror("tcp_server_init error");
        return -1;
    }

    struct event_base* base = event_base_new();

    // 监听客户端请求链接事件
    struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST, accept_cb, base);

    event_add(ev_listen, nullptr);

    event_base_dispatch(base);

    return 0;
}

void accept_cb(int fd, int16_t /*events*/, void* arg) {
    evutil_socket_t sockfd = 0;

    struct sockaddr_in client {};
    socklen_t len = sizeof(client);

    sockfd = accept4(fd, (struct sockaddr*)&client, &len, SOCK_CLOEXEC);
    evutil_make_socket_nonblocking(sockfd);

    std::cout << "accept a client " << sockfd << std::endl;

    auto* base = static_cast<event_base*>(arg);

    // 动态创建一个event结构体，并将其作为回调参数传递给
    struct event* ev = event_new(nullptr, -1, 0, nullptr, nullptr);
    event_assign(ev, base, sockfd, EV_READ | EV_PERSIST, socket_read_cb, ev);

    event_add(ev, nullptr);
}

void socket_read_cb(int fd, int16_t /*events*/, void* arg) {
    char msg[4096];
    auto* ev = static_cast<struct event*>(arg);
    int len = read(fd, msg, sizeof(msg) - 1);

    if (len <= 0) {
        std::cout << "some error happen when read" << std::endl;
        event_free(ev);
        close(fd);
        return;
    }

    msg[len] = '\0';
    std::cout << "recv the client msg: " << msg << std::endl;

    char reply_msg[4096] = "I have received the msg: ";
    strcat(reply_msg + strlen(reply_msg), msg);

    write(fd, reply_msg, strlen(reply_msg));
}

using SA = struct sockaddr;
int tcp_server_init(int port, int listen_num) {
    int errno_save = 0;
    evutil_socket_t listener = 0;

    listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        return -1;
    }

    // 允许多次绑定同一个地址。要用在socket和bind之间
    evutil_make_listen_socket_reuseable(listener);

    struct sockaddr_in sin {};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(port);

    auto err_func = [&]() {
        errno_save = errno;
        evutil_closesocket(listener);
        errno = errno_save;
        return -1;
    };

    if (::bind(listener, (SA*)&sin, sizeof(sin)) < 0) {
        err_func();
    }

    if (::listen(listener, listen_num) < 0) {
        err_func();
    }

    // 跨平台统一接口，将套接字设置为非阻塞状态
    evutil_make_socket_nonblocking(listener);

    return listener;
}
