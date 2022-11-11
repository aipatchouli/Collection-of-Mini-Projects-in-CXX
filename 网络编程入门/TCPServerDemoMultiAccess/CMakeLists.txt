cmake_minimum_required(VERSION 3.20.1)

SET(PROJECT_SOURCE_DIR /home/patchy/文档/C++_PROJECT/Collection_of_Mini_Projects_in_CXX/网络编程入门/TCPServerDemoMultiAccess)

project(TCPServerDemo)
SET(CMAKE_CXX_STANDARD 17)
# set build folder directory



#编译器相关设置
set(CMAKE_BUILD_TYPE DEBUGE)
SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_CXX_FLAGES "${CAMKE_CXX_FLAGES} -g -O2 -Wall -Wextra -fstandalone-debug -fcolor-diagnostics -std=c++17")

# 编译时从添加的路径寻找头文件
aux_source_directory(${PROJECT_SOURCE_DIR}/include INCLUDE_DIR)
include_directories(${INCLUDE_DIR})

# 添加源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src DIR_SRCS)

#设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable (TCPServerDemo ${PROJECT_SOURCE_DIR}/src/server.cxx)
add_executable (TCPClientDemo ${PROJECT_SOURCE_DIR}/src/client.cxx)