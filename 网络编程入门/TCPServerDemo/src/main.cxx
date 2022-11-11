#include <iostream>

#include "../include/bash.hpp"

int main(int argc, char *argv[]) {
  char username[BUFFSIZE] = {0};
  std::cout << "Hello world" << std::endl;

  std::cout << "Please Input Your Name :" << std::endl;

  std::cin >> username;

  std::cout << "Hi! " << username << " , Welcome China !" << std::endl;

  return 0;
}