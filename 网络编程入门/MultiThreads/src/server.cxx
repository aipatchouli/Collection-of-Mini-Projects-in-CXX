#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../include/bash.hpp"

// 出错调用函数
static void error_handle(const std::string &opt, const std::string &message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  exit(1);
}

// 子进程处理客户端请求函数封装
int ChildProcessWork(int ServSocket, int ChildSocket) {
  char buf[BUFFSIZE] = {0};
  int str_len = 0;
  // 首先在子进程中关闭服务端套接字
  close(ServSocket);

  while (true) {
    // 接收客户端数据
    str_len = read(ChildSocket, buf, BUFFSIZE);
    if (str_len == 0) {
      // 客户端断开连接，处理结束
      break;
    }
    write(ChildSocket, buf, str_len);
    memset(buf, 0, BUFFSIZE);
  }

  // 关闭客户端套接字
  close(ChildSocket);
  return 0;
}

// 处理子进程结束后，父进程回收资源的信号回调函数
void read_childproc(int signum) {
  pid_t pid = 0;
  int status = 0;
  pid = waitpid(-1, &status, WNOHANG);
  std::cout << "Removed proc id : " << pid << std::endl;
}

// 多进程工作
int fork_workproc(int serv_sock) {
  struct sockaddr_in client_adr {};
  socklen_t adr_size = 0;
  int client_sock = 0;
  int ret = 0;
  pid_t pid = 0;

  // 循环等待客户端请求
  while (true) {
    adr_size = sizeof(client_adr);
    client_sock =
        accept4(serv_sock, reinterpret_cast<struct sockaddr *>(&client_adr),
                &adr_size, SOCK_CLOEXEC);
    if (client_sock < 0) {
      // 接收请求失败，继续工作
      continue;
    }  // 接收到新的客户端，打印输出
    std::cout << "New Client IP : " << inet_ntoa(client_adr.sin_addr)
              << " , port : " << ntohs(client_adr.sin_port) << std::endl;
    // 创建子进程进行处理
    pid = fork();
    if (pid < 0) {
      // 创建子进程失败，关闭连接
      std::cout << "fork error, close client" << std::endl;
      close(client_sock);
      continue;
    }
    if (pid == 0) {
      // 进入子进程,
      ret = ChildProcessWork(serv_sock, client_sock);
      // 调用结束后直接结束子进程
      exit(ret);
    } else {
      // 父进程关闭客户端套接字，继续等待新客户端请求
      close(client_sock);
    }
  }
  return 0;
}

// select I/O复用
int select_workproc(int serv_sock) {
  struct sockaddr_in client_adr {};
  socklen_t adr_size = 0;
  int client_sock = 0;
  int ret = 0;
  int fd_max = 0;
  int fd_num = 0;
  fd_set reads;
  fd_set cpy_reads;
  struct timeval timeout {};
  char buf[BUFFSIZE];

  FD_ZERO(&reads);
  FD_SET(serv_sock, &reads);
  fd_max = serv_sock;
  int str_len = 0;

  while (true) {
    cpy_reads = reads;
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;

    fd_num = select(fd_max + 1, &cpy_reads, nullptr, nullptr, &timeout);
    if (fd_num < 0) {
      // 调用select 失败
      std::cout << "select error." << std::endl;
      break;
    }
    if (fd_num == 0) {
      // 目前没有数据事件发生
      continue;
    }
    // 循环处理当前所有可读事件
    for (int i = 0; i < fd_num; i++) {
      if (FD_ISSET(i, &cpy_reads)) {
        // 如果当前要处理的文件描述符是 serv_sock
        // 服务器套接字，则表示需要处理客户端请求连接事件
        if (i == serv_sock) {
          adr_size = sizeof(client_adr);
          client_sock = accept4(serv_sock, (struct sockaddr *)&client_adr,
                                &adr_size, SOCK_CLOEXEC);
          // 打印新连接客户端信息
          std::cout << "New Client IP : " << inet_ntoa(client_adr.sin_addr)
                    << " , port : " << ntohs(client_adr.sin_port) << std::endl;
          // 将新连接的客户端注册到select处理队列中
          FD_SET(client_sock, &reads);
          // 如果当前最大值小于新客户端套接字，则改变
          if (fd_max < client_sock) {
            fd_max = client_sock;
          }
        } else {
          // 处理客户端发来的消息，echo服务器负责将数据返回
          str_len = read(i, buf, BUFFSIZE);
          if (str_len == 0) {
            // 客户端连接断开
            // 将客户端套接字从select监控队列中清除
            FD_CLR(i, &reads);
            close(i);
            // 输出打印
            std::cout << "Closed Client : " << i << std::endl;
          } else {
            // 正常处理数据，将读取到的数据返回给客户端
            write(i, buf, str_len);
          }
        }
      }
    }
  }

  return 0;
}

// 设置套接字非阻塞
void setnonblockingmode(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

// epoll I/O 复用方式
int epoll_workproc(int serv_sock) {
  struct sockaddr_in client_adr {};
  socklen_t adr_size = 0;
  int client_sock = 0;
  int ret = 0;
  int epoll_size = 0;
  int epfd = 0;
  int event_cnt = 0;
  struct epoll_event *ep_events = nullptr;
  struct epoll_event event {};
  int timeout = 0;
  int str_len = 0;
  char buf[BUFFSIZE];

  // 初始化部分参数，创建epoll 例程
  epoll_size = 50;  // 默认定义50作为监视文件描述符数量，可修改
  epfd = epoll_create1(EPOLL_CLOEXEC);
  if (epfd < 0) {
    // 创建失败，退出程序
    std::cout << "epoll_create error." << std::endl;
    return -1;
  }
  // 申请epoll_wait 返回缓冲区
  ep_events =
      (struct epoll_event *)malloc(sizeof(struct epoll_event) * epoll_size);

  // 设置服务器套接字非阻塞
  setnonblockingmode(serv_sock);
  // 优先将服务器套接字注册到epoll监视例程中
  event.events = EPOLLIN;  // 服务器套接字不使用边缘触发
  event.data.fd = serv_sock;
  // 添加文件描述符及事件
  epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

  // 初始化超时时间为 5.5 s
  timeout = 5500;

  while (true) {
    // 调用 epoll_wait 获取当前文件描述符注册事件
    event_cnt = epoll_wait(epfd, ep_events, epoll_size, timeout);
    if (event_cnt < 0) {
      // 调用select 失败
      std::cout << "epoll_wait error." << std::endl;
      break;
    }
    if (event_cnt == 0) {
      // 目前没有数据事件发生，超时
      continue;
    }
    // 循环处理当前所有待处理事件
    for (int i = 0; i < event_cnt; i++) {
      if (ep_events[i].data.fd ==
          serv_sock)  // 判断事件文件描述符是服务器套接字
      {
        // 如果当前要处理的文件描述符是 serv_sock
        // 服务器套接字，则表示需要处理客户端请求连接事件
        adr_size = sizeof(client_adr);
        client_sock = accept4(serv_sock, (struct sockaddr *)&client_adr,
                              &adr_size, SOCK_CLOEXEC);
        // 打印新连接客户端信息
        std::cout << "New Client IP : " << inet_ntoa(client_adr.sin_addr)
                  << " , port : " << ntohs(client_adr.sin_port) << std::endl;
        // 初始化 event 变量，注册到 epoll 监视列表中
        event.events = EPOLLIN | EPOLLET;  // 添加边缘触发条件
        event.data.fd = client_sock;
        epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
      } else {
        // 由于添加了边缘触发，所以一次操作就需要将输入缓冲区中数据全部处理完成
        while (true) {
          // 处理客户端发来的消息，echo服务器负责将数据返回
          str_len = read(i, buf, BUFFSIZE);
          if (str_len == 0) {
            // 客户端断开连接，将该客户端套接字从监视列表中删除
            epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, nullptr);
            close(ep_events[i].data.fd);
            // 输出打印
            std::cout << "Closed Client : " << ep_events[i].data.fd
                      << std::endl;
            break;
          }
          if (str_len < 0) {
            if (errno == EAGAIN) {
              // 数据读取完毕
              break;
            }
          } else {
            // 正常处理数据，将读取到的数据返回给客户端
            write(ep_events[i].data.fd, buf, str_len);
          }
        }
      }
    }
  }

  // 关闭epoll 例程
  close(epfd);

  return 0;
}

void *handle_client(void *arg) {
  int client_sock = *(int *)arg;
  int str_len = 0;
  char buf[BUFFSIZE];

  while (true) {
    // 处理客户端发来的消息，echo服务器负责将数据返回
    str_len = read(client_sock, buf, BUFFSIZE);
    if (str_len == 0) {
      // 客户端连接断开
      close(client_sock);
      // 输出打印
      std::cout << "Closed Client : " << client_sock << std::endl;
    } else {
      // 正常处理数据，将读取到的数据返回给客户端
      write(client_sock, buf, str_len);
    }
  }
}

int pthread_workproc(int serv_sock) {
  struct sockaddr_in client_adr {};
  socklen_t adr_size = 0;
  int client_sock = 0;
  int ret = 0;
  pthread_t t_id = 0;

  // 循环等待客户端请求
  while (true) {
    adr_size = sizeof(client_adr);
    client_sock = accept4(serv_sock, (struct sockaddr *)&client_adr, &adr_size,
                          SOCK_CLOEXEC);

    pthread_create(&t_id, nullptr, handle_client, (void *)&client_sock);
    // 分离线程，使其单独执行，执行完成后自动结束，不需要去执行
    // pthread_join回收资源
    pthread_detach(t_id);
    // 打印新连接客户端信息
    std::cout << "New Client IP : " << inet_ntoa(client_adr.sin_addr)
              << " , port : " << ntohs(client_adr.sin_port) << std::endl;
  }

  return 0;
}

// 修改主函数
int main(int argc, char *argv[]) {
  int serv_sock = 0;
  int ret = 0;
  struct sockaddr_in serv_adr {};
  struct sigaction act {};

  int str_len = 0;
  int state = 0;
  int option = 0;
  socklen_t optlen = 0;

  if (argc < 2) {
    std::cout << "Usage : " << argv[0] << "<port> " << std::endl;
    exit(1);
  }

  act.sa_handler = read_childproc;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  state = sigaction(SIGCHLD, &act, nullptr);

  // 创建服务socket套接字
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock < 0) {
    error_handle("socket", "socket() error.");
  }

  // 添加 Time-wait 状态重新分配断开设置
  optlen = sizeof(option);
  option = 1;
  setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

  // 初始化服务套接字
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  // 绑定套接字
  ret = bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
  if (ret < 0) {
    error_handle("bind", "bind() error.");
  }

  // 监听端口
  ret = listen(serv_sock, 5);
  if (ret < 0) {
    error_handle("listen", "listen() error");
  }

  // //多进程方式实现
  // fork_workproc(serv_sock);

  // //select I/O 复用方式
  // select_workproc(serv_sock);

  // epoll I/O 复用方式
  //  epoll_workproc(serv_sock);

  // 多线程方式
  pthread_workproc(serv_sock);

  // 关闭套接字
  close(serv_sock);
  return 0;
}