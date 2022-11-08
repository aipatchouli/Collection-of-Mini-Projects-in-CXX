#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int tcp_connect_server(const char* server_ip, int port);

void cmd_msg_cb(int fd, int16_t events, void* arg);
void server_msg_cb(struct bufferevent* bev, void* arg);
void event_cb(struct bufferevent* bev, int16_t event, void* arg);

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "please input server ip and port" << std::endl;
        return -1;
    }

    // 两个参数依次是服务器端的IP地址、端口号
    int sockfd = tcp_connect_server(argv[1], atoi(argv[2]));
    if (sockfd == -1) {
        perror("tcp_connect error ");
        return -1;
    }

    std::cout << "connect to server success" << std::endl;

    struct event_base* base = event_base_new();

    struct bufferevent* bev = bufferevent_socket_new(base, sockfd,
                                                     BEV_OPT_CLOSE_ON_FREE);

    // 监听终端输入事件
    struct event* ev_cmd = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, cmd_msg_cb, (void*)bev);
    event_add(ev_cmd, nullptr);

    // 当socket关闭时会用到回调参数
    bufferevent_setcb(bev, server_msg_cb, nullptr, event_cb, (void*)ev_cmd);
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

    event_base_dispatch(base);

    std::cout << "finished" << std::endl;

    return 0;
}

void cmd_msg_cb(int fd, int16_t /*events*/, void* arg) {
    char msg[1024];

    int ret = read(fd, msg, sizeof(msg));
    if (ret < 0) {
        perror("read fail");

        _exit(1);
    }

    struct bufferevent* bev = (struct bufferevent*)arg;

    // 把终端的消息发送给服务器端
    bufferevent_write(bev, msg, ret);
}

void server_msg_cb(struct bufferevent* bev, void* /*arg*/) {
    char msg[1024];

    size_t len = bufferevent_read(bev, msg, sizeof(msg));
    msg[len] = '\0';

    std::cout << "recv " << msg << " from server" << std::endl;
}

void event_cb(struct bufferevent* bev, int16_t event, void* arg) {
    if ((event & BEV_EVENT_EOF) != 0) {
        std::cout << "connection closed" << std::endl;

    } else if ((event & BEV_EVENT_ERROR) != 0) {
        std::cout << "some other error" << std::endl;
    }

    // 这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);

    struct event* ev = (struct event*)arg;
    // 因为socket已经没有，所以这个event也没有存在的必要了
    event_free(ev);
}

using SA = struct sockaddr;
int tcp_connect_server(const char* server_ip, int port) {
    int sockfd = 0;
    int status = 0;
    int save_errno = 0;
    struct sockaddr_in server_addr {};

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    status = inet_aton(server_ip, &server_addr.sin_addr);

    if (status == 0) // the server_ip is not valid value
    {
        errno = EINVAL;
        return -1;
    }

    sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return sockfd;
    }

    status = ::connect(sockfd, (SA*)&server_addr, sizeof(server_addr));

    if (status == -1) {
        save_errno = errno;
        ::close(sockfd);
        errno = save_errno; // the close may be error
        return -1;
    }

    evutil_make_socket_nonblocking(sockfd);

    return sockfd;
}