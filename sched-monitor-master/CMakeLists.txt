# CMakeLists.txt - User-space tools build configuration
#
# Linux Kernel Performance Monitoring Platform
#
# Copyright (C) 2024

cmake_minimum_required(VERSION 3.16)
project(sched_monitor_user VERSION 1.0.0 LANGUAGES C)

# ========== Compiler Settings ==========
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# ========== Include Directories ==========
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/user/common
)

# ========== Common Library ==========
add_library(sched_monitor_common STATIC
    user/common/netlink_client.c
    user/common/stats_processor.c
)

target_link_libraries(sched_monitor_common
    pthread
)

# ========== monitord ==========
add_executable(monitord
    user/monitord/monitord.c
)

target_link_libraries(monitord
    sched_monitor_common
    pthread
)

# ========== schedtop ==========
add_executable(schedtop
    user/schedtop/schedtop.c
)

target_link_libraries(schedtop
    sched_monitor_common
    pthread
)

# ========== Install Targets ==========
install(TARGETS monitord schedtop
    RUNTIME DESTINATION bin
)

install(TARGETS sched_monitor_common
    ARCHIVE DESTINATION lib
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/include/sched_monitor.h
    ${CMAKE_SOURCE_DIR}/user/common/user_common.h
    DESTINATION include
)

# ========== Print Configuration ==========
message(STATUS "")
message(STATUS "sched_monitor_user Configuration:")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  C Compiler:  ${CMAKE_C_COMPILER}")
message(STATUS "  Build Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "")
