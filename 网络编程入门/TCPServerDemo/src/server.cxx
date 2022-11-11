#include <arpa/inet.h>
#include <sys/socket.h>
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

int main(int argc, char *argv[]) {
  int serv_sock = 0;
  int client_sock = 0;
  char message[BUFFSIZE];
  int str_len = 0;
  int ret = 0;

  struct sockaddr_in serv_adr {};
  struct sockaddr_in client_adr {};
  socklen_t client_adr_size = 0;

  // 判断命令行参数合法性
  if (argc < 2) {
    std::cout << "Usage : " << argv[0] << " <port>" << std::endl;
    _exit(1);
  }

  // 创建socket套接字
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock < 0) {
    error_handle("socket", "socket() error.");
  }

  // 初始化服务器套接字参数，设置网卡IP 和端口号
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  // 绑定端口
  ret = bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
  if (ret < 0) {
    error_handle("bind", "bind() error.");
  }

  // 监听TCP端口号
  ret = listen(serv_sock, 5);
  if (ret < 0) {
    error_handle("listen", "listen() error.");
  }

  client_adr_size = sizeof(client_adr);

  while (true) {
    // 使用accept接收客户端连接请求
    client_sock = accept4(serv_sock, (struct sockaddr *)&client_adr,
                          &client_adr_size, SOCK_CLOEXEC);
    if (client_sock < 0) {
      // 接收客户端连接请求失败，
      continue;
    }
    // 接收新的客户端请求，进行客户端数据处理
    std::cout << "Accept New Client : " << inet_ntoa(client_adr.sin_addr)
              << " , port : " << ntohs(client_adr.sin_port) << std::endl;
    std::cout << "Start Recv Client Data..." << std::endl;

    // 清空缓存区
    memset((void *)&message, 0, BUFFSIZE);
    while ((str_len = read(client_sock, message, BUFFSIZE)) != 0) {
      // 成功读取客户端发送来的数据消息
      // 打印输出
      std::cout << "Recv Message : " << message << std::endl;
      // 将消息回传给客户端，作为回声服务器，类似 echo 命令
      write(client_sock, message, str_len);
      // 清空缓存区，等待再次读取数据
      memset(message, 0, BUFFSIZE);
    }

    // 客户端断开连接，关闭套接字
    close(client_sock);
  }

  // 服务器关闭，关闭服务器套接字
  close(serv_sock);
  return 0;
}