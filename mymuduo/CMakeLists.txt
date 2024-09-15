cmake_minimum_required(VERSION 3.6)
project(mymuduo)

# mymuduo最终编译成so库, 设置动态库的路径, 放在根目录下lib中
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
#编译生成动态库libmymuduo.so
add_library(mymuduo SHARED ${SRC_LIST})
