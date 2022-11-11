#include <arpa/inet.h>
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
  _exit(1);
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
    // 多进程服务中不再将受到数据打印输出
    // 将客户端数据返回给客户端
    write(ChildSocket, buf, str_len);
    memset(buf, 0, BUFFSIZE);
  }

  // 关闭客户端套接字
  close(ChildSocket);
  return 0;
}

// 处理子进程结束后，父进程回收资源的信号回调函数
void read_childproc(int /*signum*/) {
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
    client_sock = accept4(serv_sock, (struct sockaddr *)&client_adr, &adr_size,
                          SOCK_CLOEXEC);
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
      _exit(ret);
    } else {
      // 父进程关闭客户端套接字，继续等待新客户端请求
      close(client_sock);
    }
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
  char buf[BUFFSIZE];

  if (argc < 2) {
    std::cout << "Usage : " << argv[0] << "<port> " << std::endl;
    _exit(1);
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

  fork_workproc(serv_sock);

  // 关闭套接字
  close(serv_sock);
  return 0;
}