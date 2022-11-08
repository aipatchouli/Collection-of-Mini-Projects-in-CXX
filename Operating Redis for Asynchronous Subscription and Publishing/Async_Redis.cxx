// 使用hiredis链接redis服务器，实现异步消息的订阅和发布
#include <hiredis/hiredis.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

