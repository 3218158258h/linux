cmake_minimum_required(VERSION 3.10)


set(pub_exe publisher)  # 发布者程序名
set(sub_exe subscriber)# 订阅者程序名
set(head HelloWorldData_)     # 头文件,由IDL生成
project(cyclone_ros2_publisher_subscriber)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# dds库的路径
set(CYCLONEDDS_HOME /home/cyclonedds)#cyclonedds
set(CYCLONEDDS_CXX_HOME /home/cyclonedds-cxx)#cyclonedds-cxx

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}  # 工程根目录
    ${CYCLONEDDS_HOME}/include
    ${CYCLONEDDS_CXX_HOME}/include
)

# 库文件目录（公共目录，两个程序共用）
link_directories(
    ${CYCLONEDDS_HOME}/lib
    ${CYCLONEDDS_CXX_HOME}/lib
)

# 编译发布者程序
add_executable(${pub_exe} 
    ${pub_exe}.cpp    # 发布者源文件: publisher.cpp
    ${head}.cpp             # 手动编译好的IDL实现文件
)

# 链接发布者程序所需库
target_link_libraries(${pub_exe}
    ${CYCLONEDDS_CXX_HOME}/lib/libddscxx.so
    ${CYCLONEDDS_HOME}/lib/libddsc.so
    pthread
    m
)

# 编译订阅者程序
add_executable(${sub_exe} 
    ${sub_exe}.cpp   # 订阅者源文件: subscriber.cpp
    ${head}.cpp             # 手动编译好的IDL实现文件（和发布者共用）
)

# 链接订阅者程序所需库（和发布者共用相同的库）
target_link_libraries(${sub_exe}
    ${CYCLONEDDS_CXX_HOME}/lib/libddscxx.so
    ${CYCLONEDDS_HOME}/lib/libddsc.so
    pthread
    m
)