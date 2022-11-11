#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <event.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <unistd.h>

void accept_cb(int fd, int16_t events, void* arg);
void socket_read_cb(bufferevent* bev, void* arg);
void event_cb(struct bufferevent* bev, int16_t event, void* arg);
int tcp_server_init(int port, int listen_num);

int main(int /*argc*/, char** /*argv*/) {
    int listener = tcp_server_init(9999, 10);
    if (listener == -1) {
        perror(" tcp_server_init error ");
        return -1;
    }

    struct event_base* base = event_base_new();

    // 添加监听客户端请求连接事件
    struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST,
                                        accept_cb, base);
    event_add(ev_listen, nullptr);

    event_base_dispatch(base);
    event_base_free(base);

    return 0;
}

void accept_cb(int fd, int16_t /*events*/, void* arg) {
    evutil_socket_t sockfd = 0;

    struct sockaddr_in client {};
    socklen_t len = sizeof(client);

    sockfd = accept4(fd, (struct sockaddr*)&client, &len, SOCK_CLOEXEC);
    evutil_make_socket_nonblocking(sockfd);

    std::cout << "accept a client " << sockfd << std::endl;

    struct event_base* base = static_cast<event_base*>(arg);

    bufferevent* bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, socket_read_cb, nullptr, event_cb, arg);

    bufferevent_enable(bev, EV_READ | EV_PERSIST);
}

void socket_read_cb(bufferevent* bev, void* /*arg*/) {
    char msg[4096];

    size_t len = bufferevent_read(bev, msg, sizeof(msg));

    msg[len] = '\0';
    std::cout << "recv the client msg: " << msg << std::endl;

    char reply_msg[4096] = "I have recvieced the msg: ";

    strcat(reply_msg + strlen(reply_msg), msg);
    bufferevent_write(bev, reply_msg, strlen(reply_msg));
}

void event_cb(struct bufferevent* bev, int16_t event, void* /*arg*/) {
    if ((event & BEV_EVENT_EOF) != 0) {
        std::cout << "connection closed" << std::endl;
    } else if ((event & BEV_EVENT_ERROR) != 0) {
        std::cout << "some other error" << std::endl;
    }

    // 这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
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
