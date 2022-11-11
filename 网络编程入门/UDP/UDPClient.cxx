#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#define BUF_SIZE 128

// 出错调用函数
void error_handle(const std::string &opt, const std::string &message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  _exit(1);
}

int main(int argc, char *argv[]) {
  int sock = 0;
  char message[BUF_SIZE];
  int str_len = 0;
  socklen_t adr_sz = 0;
  struct sockaddr_in serv_adr {};
  struct sockaddr_in from_adr {};

  // 因为客户端程序使用时通过命令行参数指定IP地址和端口号，
  // 所以当参数数量少于需求 3 时，程序结束并报错
  if (argc < 3) {
    std::cout << "Usage : " << argv[0] << " <IP> <port>" << std::endl;
    _exit(0);
  }

  // 创建socket 套接字，sock 用于后面发送和接收数据
  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    error_handle("socket", "socket() error.");
  }

  // 初始化套接字结构体，初始化服务器套接字结构体中的 IP 地址和端口号
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  while (true) {
    // 从标准输入获取发送数据，保存到message缓存区
    std::cin >> message;
    // 判断如果输入是 'q' 或者 'Q' 则表示退出客户端程序
    if ((strcmp(message, "q\n") == 0) || (strcmp(message, "Q\n") == 0)) {
      // 退出
      break;
    }

    // 发送消息
    sendto(sock, message, strlen(message), 0, (struct sockaddr *)&serv_adr,
           sizeof(serv_adr));

    // 清空接收数组
    memset(message, 0, BUF_SIZE);
    // 计算返回套接字结构体字节长度
    adr_sz = sizeof(from_adr);
    // 接收UDP返回数据
    str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr *)&from_adr,
                       &adr_sz);
    // 在接收数据尾部添加'0'结束符
    message[str_len + 1] = 0;
    // 将接收数据打印输出
    std::cout << "Recv Message : " << message << std::endl;
  }
}