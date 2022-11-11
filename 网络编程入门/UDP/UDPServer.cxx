#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#define BUF_SIZE 128

// 出错调用函数
void error_handle(const std::string& opt, const std::string& message) {
  // 根据errno值获取失败原因并打印到终端
  perror(opt.c_str());
  std::cout << message << std::endl;
  _exit(1);
}

int main(int argc, char *argv[]) {
  int serv_sock = 0;
  char message[BUF_SIZE];
  int str_len = 0;
  socklen_t client_adr_sz = 0;
  struct sockaddr_in serv_adr{};
  struct sockaddr_in client_adr{};

  // 因为服务器程序使用时通过命令行参数指定接收消息的端口号，
  // 所以当参数数量少于需求 2 时，程序结束并报错
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <port>" << std::endl;
    _exit(0);
  }

  // 创建socket 套接字，sock 用于后面发送和接收数据
  serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (serv_sock == -1) {
    error_handle("socket", "UDP socket creation error.");
  }

  // 初始化套接字结构体，初始化服务器套接字结构体中的 IP 地址和端口号
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  // 将socket套接字绑定固定端口进行消息接收
  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) {
    error_handle("bind", "bind() error.");
  }

  // UDP 方式不需要进行 listen 和 accept 操作，直接就可以通过 recvfrom
  // 函数去接收套接字中收到的数据。
  while (true) {
    // 清空接收消息数据的缓存区
    memset(message, 0, BUF_SIZE);
    // 计算客户端结构体大小，用于接收客户端数据结构
    client_adr_sz = sizeof(client_adr);
    // recvfrom() 调用不仅收到对方发送来的数据，还通过最后两个参数返回了对方的
    // IP 地址和端口号信息，用于返回数据使用。
    str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,
                       (struct sockaddr *)&client_adr, &client_adr_sz);
    // 将收到的信息打印出来
    std::cout << "Recv Message : " << message << std::endl;
    // 通过recvfrom 返回的发送方的IP地址和端口号信息，使用 sendto
    // 将收到的信息再次发送回去。
    sendto(serv_sock, message, str_len, 0, (struct sockaddr *)&client_adr,
           client_adr_sz);
  }

  // 结束程序后关闭socket套接字
  close(serv_sock);

  return 0;
}