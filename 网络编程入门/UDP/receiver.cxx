#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
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
  int recv_sock = 0;
  struct sockaddr_in adr{};
  struct ip_mreq join_adr{};
  int str_len = 0;
  char buf[BUFFSIZE];

  // 判断当前命令行参数是否有效
  if (argc < 3) {
    std::cout << "Usage : " << argv[0] << "Group IP <PORT>" << std::endl;
    _exit(1);
  }

  // 创建 UDP socket套接字
  recv_sock = socket(PF_INET, SOCK_DGRAM, 0);

  // 初始化UDP 套接字及端口号
  memset(&adr, 0, sizeof(adr));
  adr.sin_family = AF_INET;
  adr.sin_addr.s_addr = htonl(INADDR_ANY);
  adr.sin_port = htons(atoi(argv[2]));

  // 绑定接收数据的 UDP 端口
  if (bind(recv_sock, (struct sockaddr *)&adr, sizeof(adr)) == 1) {
    error_handle("bind", "bind() error.");
  }

  join_adr.imr_multiaddr.s_addr = inet_addr(argv[1]);
  join_adr.imr_interface.s_addr = htonl(INADDR_ANY);

  setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_adr,
             sizeof(join_adr));

  // 循环接收多播数据并打印输出
  while (true) {
    memset(buf, 0, sizeof(buf));
    str_len = recvfrom(recv_sock, buf, BUFFSIZE - 1, 0, nullptr, nullptr);
    if (str_len < 0) {
      break;
    }
    buf[str_len] = 0;
    std::cout << buf << std::endl;
  }

  close(recv_sock);
  return 0;
}