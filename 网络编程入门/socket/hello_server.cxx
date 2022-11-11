#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// 出错调用函数
void error_handle(const std::string &opt, const std::string &message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  _exit(1);
}

int main(int argc, char *argv[]) {
  int serv_sock = 0;
  int client_sock = 0;

  struct sockaddr_in serv_addr {};  // 定义socket地址结构
  struct sockaddr_in client_addr {};

  socklen_t client_addr_size = 0;
  char message[] = "hello world";

  // 判断参数数量，Usage: <port>， 需要在命令行输入服务器接收消息的端口号
  if (argc < 2) {
    std::cout << "Usage : " << argv[0] << " <port>" << std::endl;
    _exit(1);
  }

  // 创建socket 套接字
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock < 0) {
    error_handle("socket", "socket() error.");
  }

  // 初始化套接字结构体
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 选择当前任意网卡
  serv_addr.sin_port = htons(atoi(argv[1]));  // 设置接收消息的端口号

  // 绑定端口
  if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    error_handle("bind", "bind() error.");
  }

  // 监听端口，设置等待队列数量为5
  if (listen(serv_sock, 5) < 0) {
    error_handle("listen", "listen() error.");
  }

  // 打印输出等待连接
  std::cout << "Waiting Client...." << std::endl;

  client_addr_size = sizeof(client_addr);
  // 等待接收客户端建立连接
  client_sock =
      accept(serv_sock, (struct sockaddr *)&client_sock, &client_addr_size);
  if (client_sock < 0) {
    error_handle("accept", "accept() error.");
  }
  // accept() 成功建立连接后，服务器就会得到客户端的 IP 地址和端口号。
  // 打印客户端 IP 和端口号
  std::cout << "Client IP : " << inet_ntoa(client_addr.sin_addr)
            << " , port : " << ntohs(client_addr.sin_port) << std::endl;

  // 向客户端发送 "hello world" 消息，使用write 标准IO接口就可以
  write(client_sock, message, sizeof(message));

  // 关闭TCP连接
  close(client_sock);
  // 关闭socket套接字
  close(serv_sock);

  return 0;
}