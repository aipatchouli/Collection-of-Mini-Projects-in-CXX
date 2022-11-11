#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#define BUFFSIZE 1024

int main(int /*argc*/, char* /*argv*/[]) {
  fd_set reads;
  fd_set temps;
  int result = 0;
  int str_len = 0;
  char buf[BUFFSIZE];
  struct timeval timeout {};
  FD_ZERO(&reads);
  FD_SET(0, &reads);

  while (true) {
    temps = reads;
    // 设置超时时间 5.5 s
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;
    // 清空缓冲区内容
    memset(buf, 0, BUFFSIZE);

    result = select(1, &temps, nullptr, nullptr, &timeout);
    if (result < 0) {
      std::cout << "select() error. " << std::endl;
      break;
    }
    if (result == 0) {
      // 超时
      std::cout << "Time-out!" << std::endl;
      continue;
    }
    // 判断是否是标准输入有数据到
    if (FD_ISSET(0, &temps)) {
      str_len = read(0, buf, BUFFSIZE);
      buf[str_len] = 0;
      std::cout << "Message from console : " << buf << std::endl;
    }
  }

  return 0;
}