#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>

#define TTL 64
#define BUFFSIZE 64

// 出错调用函数
void error_handle(const std::string& opt, const std::string& message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  _exit(1);
}

int main(int argc, char *argv[]) {
  // 声明变量
  int send_sock = 0;
  struct sockaddr_in mul_adr{};
  int time_live = TTL;
  FILE *fp = nullptr;
  char buf[BUFFSIZE];

  if (argc < 3) {
    std::cout << "Usage : " << argv[0] << "Group IP <Port>" << std::endl;
    _exit(1);
  }

  // 创建 UDP 套接字
  send_sock = socket(PF_INET, SOCK_DGRAM, 0);

  // 套接字结构体初始化
  memset(&mul_adr, 0, sizeof(mul_adr));
  mul_adr.sin_family = AF_INET;
  mul_adr.sin_addr.s_addr = inet_addr(argv[1]);
  mul_adr.sin_port = htons(atoi(argv[2]));

  // 设置 socketopt 选项为多播模式
  setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live,
             sizeof(time_live));

  // 以文本模式打开in.txt备读
  std::ifstream srcFile("news.txt", std::ios::in);
  if (!srcFile) {  // 打开失败
    error_handle("ifstream", "ifstream error.");
  }

  // 可以像用cin那样用ifstream对象
  memset(buf, 0, BUFFSIZE);
  while (srcFile >> buf) {
    sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr *)&mul_adr,
           sizeof(mul_adr));
    memset(buf, 0, BUFFSIZE);
  }

  // 关闭套接字
  close(send_sock);

  std::cout << "数据发送完毕！" << std::endl;
  return 0;
}