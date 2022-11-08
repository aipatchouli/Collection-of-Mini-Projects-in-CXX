
/**
You need libevent2 to compile this piece of code
Please see: http://libevent.org/
Or you can simply run this command to install on Mac: brew install libevent
Cmd to compile this piece of code: g++ LibeventQuickStartClient.c -o LibeventQuickStartClient /usr/local/lib/libevent.a
**/
#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <event.h>
#include <event2/util.h>
#include <iostream>

int tcp_connect_server(const char* server_ip, int port);

void cmd_msg_cb(int fd, int16_t events, void* arg);
void socket_read_cb(int fd, int16_t events, void* arg);

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "please input 2 parameter" << std::endl;
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

    struct event* ev_sockfd = event_new(base, sockfd,
                                        EV_READ | EV_PERSIST,
                                        socket_read_cb, nullptr);
    event_add(ev_sockfd, nullptr);

    // 监听终端输入事件
    struct event* ev_cmd = event_new(base, STDIN_FILENO,
                                     EV_READ | EV_PERSIST, cmd_msg_cb,
                                     (void*)&sockfd);

    event_add(ev_cmd, nullptr);

    event_base_dispatch(base);

    std::cout << "finished" << std::endl;

    return 0;
}

void cmd_msg_cb(int fd, int16_t /*events*/, void* arg) {
    char msg[1024];

    int ret = read(fd, msg, sizeof(msg));
    if (ret <= 0) {
        perror("read fail ");
        exit(1);
    }

    int sockfd = *(static_cast<int*>(arg));

    // 把终端的消息发送给服务器端
    // 为了简单起见，不考虑写一半数据的情况
    write(sockfd, msg, ret);
}

void socket_read_cb(int fd, int16_t /*events*/, void* /*arg*/) {
    char msg[1024];

    // 为了简单起见，不考虑读一半数据的情况
    int len = read(fd, msg, sizeof(msg) - 1);
    if (len <= 0) {
        perror("read fail ");
        exit(1);
    }

    msg[len] = '\0';

    printf("recv %s from server\n", msg);
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
