#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

// 出错调用函数
void error_handle(const std::string& opt, const std::string& message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  _exit(1);
}

int main(int argc, char *argv[]) {
  int sock = 0;                      // socket套接字
  struct sockaddr_in serv_addr{};  // 服务器套接字结构体
  char message[64];              // 用于接收服务器消息的缓冲区
  int str_len = 0;

  // 判断当前参数数量，需要命令行参数 <IP> <port>
  if (argc < 3) {
    std::cout << "Usage : " << argv[0] << " <IP> <port>" << std::endl;
    _exit(1);
  }

  // 创建socket 套接字
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    error_handle("socket", "socket() error.");
  }

  // 初始化服务器套接字结构体参数，设置对方的IP地址和端口号
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  // 与服务器建立连接
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    error_handle("connect", "connect() error.");
  }

  // 读取服务器发送来的消息
  str_len = read(sock, message, sizeof(message) - 1);
  if (str_len < 0) {
    // read() 读取数据失败
    error_handle("read", "read() error.");
  }

  // 将读取到的输出打印出来
  std::cout << "Recv Message : " << message << std::endl;

  // 关闭socket 套接字
  close(sock);
  return 0;
}