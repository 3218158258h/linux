cmake_minimum_required(VERSION 3.10)
project(cyclone_ros2_publisher)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

set(exe publisher)#可执行程序
set(head HelloWorldData_)#头文件,由IDL生成

# dds库的路径
set(CYCLONEDDS_HOME /home/cyclonedds)#cyclonedds
set(CYCLONEDDS_CXX_HOME /home/cyclonedds-cxx)#cyclonedds-cxx

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}  # 工程根目录
    ${CYCLONEDDS_HOME}/include
    ${CYCLONEDDS_CXX_HOME}/include
)

# 库文件目录
link_directories(
    ${CYCLONEDDS_HOME}/lib
    ${CYCLONEDDS_CXX_HOME}/lib
)

# 编译程序
add_executable(${exe} 
    ${exe}.cpp    # 发布者程序
    ${head}.cpp      # 手动编译好的IDL实现文件
)

# 链接库
target_link_libraries(${exe}
    ${CYCLONEDDS_CXX_HOME}/lib/libddscxx.so
    ${CYCLONEDDS_HOME}/lib/libddsc.so
    pthread
    m
)