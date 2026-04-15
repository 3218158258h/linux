```bash
# Linux内核性能监控平台 (sched-monitor)

一个基于Linux内核tracepoint的调度性能监控系统，用于实时捕获和分析调度延迟、调度事件和异常情况。

## 目录

- [项目简介](#项目简介)
- [系统要求](#系统要求)
- [快速开始](#快速开始)
- [详细编译步骤](#详细编译步骤)
- [使用方法](#使用方法)
- [项目架构](#项目架构)
- [技术细节](#技术细节)
- [常见问题解答](#常见问题解答)
- [面试相关](#面试相关)
- [开发指南](#开发指南)

## 项目简介

sched-monitor是一个高性能的Linux内核调度监控系统，具有以下特点：

- **零侵入性监控**：基于tracepoint机制，无需修改内核源码
- **低开销设计**：Per-CPU环形缓冲区，无锁写入，最小化对调度路径的影响
- **实时分析**：支持实时统计、异常检测和延迟分布直方图
- **灵活配置**：支持PID过滤、阈值配置、动态启停
- **可视化工具**：提供类似top的交互式监控界面

### 主要功能

1. **调度事件捕获**：捕获sched_switch、sched_wakeup等调度事件
2. **延迟统计**：计算调度延迟、运行时间等关键指标
3. **异常检测**：基于阈值检测调度异常（如延迟过大）
4. **进程分析**：按进程统计延迟、切换次数、异常次数
5. **延迟分布**：16级直方图展示延迟分布情况

## 系统要求

### 硬件要求

- CPU：支持SMP的多核处理器
- 内存：至少512MB可用内存
- 存储：至少100MB可用空间

### 软件要求

- **操作系统**：Linux 4.19+（推荐5.4+）
- **内核头文件**：`linux-headers-$(uname -r)`
- **编译工具链**：
  - GCC 7.0+
  - Make 3.81+
  - CMake 3.16+
- **开发工具**：
  - kernel-devel / linux-headers
  - build-essential

### 依赖安装

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y build-essential cmake linux-headers-$(uname -r) stress-ng
```

**CentOS/RHEL:**
```bash
sudo yum install -y gcc make cmake kernel-devel kernel-headers stress-ng
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake linux-headers stress
```

## 快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd sched-monitor
```

### 2. 编译项目

```bash
make all
```

### 3. 加载内核模块

```bash
sudo insmod kernel/sched_monitor.ko
```

### 4. 启动监控

```bash
# 启动守护进程
./build/monitord -d -l /tmp/monitord.log -p /tmp/monitord.pid

# 运行交互式监控工具
./build/schedtop
```

### 5. 卸载模块

```bash
sudo rmmod sched_monitor
```

## 详细编译步骤

### 步骤1：检查环境

首先检查内核头文件是否已安装：

```bash
$ uname -r
5.15.0-91-generic

$ ls -la /lib/modules/$(uname -r)/build
drwxr-xr-x  7 root root 4096 Jan 15 10:30 .
drwxr-xr-x  3 root root 4096 Jan 15 10:30 ..
-rw-r--r--  1 root root  123 Jan 15 10:30 Makefile
drwxr-xr-x  2 root root 4096 Jan 15 10:30 include
...
```

如果头文件不存在，请先安装：

```bash
sudo apt install linux-headers-$(uname -r)
```

### 步骤2：编译内核模块

```bash
$ cd kernel
$ make clean
make: Entering directory '/lib/modules/5.15.0-91-generic/build'
  CLEAN   /home/user/sched-monitor/kernel/.tmp_versions
  CLEAN   /home/user/sched-monitor/kernel/Module.symvers
  CLEAN   /home/user/sched-monitor/kernel/modules.order
  CLEAN   /home/user/sched-monitor/kernel
make: Leaving directory '/lib/modules/5.15.0-91-generic/build'

$ make
make: Entering directory '/lib/modules/5.15.0-91-generic/build'
  CC [M]  /home/user/sched-monitor/kernel/main.o
  CC [M]  /home/user/sched-monitor/kernel/trace_hook.o
  CC [M]  /home/user/sched-monitor/kernel/ring_buffer.o
  CC [M]  /home/user/sched-monitor/kernel/netlink_comm.o
  CC [M]  /home/user/sched-monitor/kernel/async_worker.o
  CC [M]  /home/user/sched-monitor/kernel/config.o
  CC [M]  /home/user/sched-monitor/kernel/stats.o
  MODPOST /home/user/sched-monitor/kernel/Module.symvers
  CC [M]  /home/user/sched-monitor/kernel/sched_monitor.mod.o
  LD [M]  /home/user/sched-monitor/kernel/sched_monitor.ko
  BTF [M] /home/user/sched-monitor/kernel/sched_monitor.ko
Skipping BTF generation for /home/user/sched-monitor/kernel/sched_monitor.ko due to unavailability of vmlinux
make: Leaving directory '/lib/modules/5.15.0-91-generic/build'

$ ls -lh sched_monitor.ko
-rw-r--r-- 1 user user 58K Jan 15 10:35 sched_monitor.ko
```

### 步骤3：编译用户态工具

```bash
$ cd ..
$ mkdir -p build && cd build
$ cmake ..
-- The C compiler identification is GNU 11.4.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done

sched_monitor_user Configuration:
  Version:     1.0.0
  C Compiler:  /usr/bin/cc
  Build Type:  Release

-- Configuring done
-- Generating done
-- Build files have been written to: /home/user/sched-monitor/build

$ make
Scanning dependencies of target sched_monitor_common
[ 25%] Building C object CMakeFiles/sched_monitor_common.dir/user/common/netlink_client.c.o
[ 50%] Building C object CMakeFiles/sched_monitor_common.dir/user/common/stats_processor.c.o
[ 50%] Linking C static library libsched_monitor_common.a
[ 75%] Building C object CMakeFiles/monitord.dir/user/monitord/monitord.c.o
[ 75%] Linking C executable monitord
[100%] Building C object CMakeFiles/schedtop.dir/user/schedtop/schedtop.c.o
[100%] Linking C executable schedtop

$ ls -lh
-rwxr-xr-x 1 user user 45K Jan 15 10:38 monitord
-rwxr-xr-x 1 user user 52K Jan 15 10:38 schedtop
```

### 步骤4：验证编译结果

```bash
$ modinfo kernel/sched_monitor.ko
filename:       /home/user/sched-monitor/kernel/sched_monitor.ko
version:        1.0.0
description:    Linux Kernel Performance Monitoring Platform
author:         Your Name
license:        GPL
srcversion:     4A2B3C4D5E6F7A8B9C0D1E2
depends:        
retpoline:      Y
name:           sched_monitor
vermagic:       5.15.0-91-generic SMP mod_unload modversions 
parm:           buffer_size:Per-CPU buffer size (number of events) (uint)
parm:           interval_ms:Workqueue execution interval (milliseconds) (uint)
parm:           threshold_ns:Anomaly detection threshold (nanoseconds) (ulong)
parm:           filter_pid:PID filter (0 = no filter) (uint)
parm:           enabled:Enable monitoring on load (bool)

$ ./build/monitord --help
Usage: ./build/monitord [OPTIONS]

Options:
  -d, --daemon       Run as daemon
  -l, --log FILE     Log file path
  -p, --pid FILE     PID file path
  -v, --verbose      Increase verbosity
  -q, --quiet        Decrease verbosity
  -h, --help         Show this help

$ ./build/schedtop --help
Usage: ./build/schedtop [OPTIONS]

Options:
  -i, --interval N    Refresh interval in seconds (default: 1)
  -n, --top N         Show top N processes (default: 10)
  -p, --pid PID       Filter by PID
  -H, --histogram     Show histogram on startup
  -h, --help          Show this help
```

### 步骤5：使用顶层Makefile（推荐）

```bash
$ make help
Linux Kernel Performance Monitoring Platform

Usage: make [target]

Targets:
  all            Build kernel module and user tools (default)
  kernel         Build kernel module only
  user           Build user tools only
  clean          Remove all build artifacts
  install        Install to system

Development:
  test           Build, load module, start daemon, run schedtop
  stop           Stop all components

Kernel module:
  kernel-load    Load kernel module
  kernel-unload  Unload kernel module

User tools:
  user-install   Install user tools to /usr/local/bin

$ make all
make: Entering directory '/home/user/sched-monitor/kernel'
make: Leaving directory '/home/user/sched-monitor/kernel'
mkdir -p build && cd build && cmake .. && make
[... 编译输出 ...]
```

## 使用方法

### 基本使用流程

#### 1. 加载内核模块

```bash
# 使用默认参数加载
sudo insmod kernel/sched_monitor.ko

# 使用自定义参数加载
sudo insmod kernel/sched_monitor.ko buffer_size=8192 interval_ms=5 threshold_ns=50000000

# 检查模块是否加载成功
$ lsmod | grep sched_monitor
sched_monitor         45056  0 

# 查看内核日志
$ dmesg | tail -10
[ 1234.567890] sched_monitor: Loading sched_monitor version 1.0.0
[ 1234.567891] sched_monitor: Parameters: buffer_size=4096, interval_ms=10, threshold_ns=100000000, filter_pid=0
[ 1234.567892] sched_monitor: Initializing per-CPU ring buffers
[ 1234.567893] sched_monitor: Ring buffers initialized successfully
[ 1234.567894] sched_monitor: Initializing Netlink communication
[ 1234.567895] sched_monitor: Netlink socket created (protocol=31, group=1)
[ 1234.567896] sched_monitor: Initializing async worker
[ 1234.567897] sched_monitor: Async worker initialized
[ 1234.567898] sched_monitor: Registering tracepoint probes
[ 1234.567899] sched_monitor: Tracepoint probes registered successfully
[ 1234.567900] sched_monitor: Monitoring started
[ 1234.567901] sched_monitor: Module loaded successfully
```

#### 2. 启动monitord守护进程

```bash
# 前台运行（用于调试）
./build/monitord -l /tmp/monitord.log

# 后台运行（守护进程模式）
./build/monitord -d -l /tmp/monitord.log -p /tmp/monitord.pid

# 检查守护进程状态
$ ps aux | grep monitord
user     12345  0.0  0.1  12345  6789 ?        Ssl  10:30   0:00 ./build/monitord -d -l /tmp/monitord.log -p /tmp/monitord.pid

$ cat /tmp/monitord.pid
12345

# 查看日志
$ tail -f /tmp/monitord.log
[2024-01-15 10:30:00] ========================================
[2024-01-15 10:30:00] sched_monitor daemon starting...
[2024-01-15 10:30:00] ========================================
[2024-01-15 10:30:00] Netlink client initialized (sock=3, port=12345)
[2024-01-15 10:30:00] Event processing thread started
[2024-01-15 10:30:00] Statistics thread started
```

#### 3. 运行schedtop交互式监控

```bash
# 基本使用
./build/schedtop

# 自定义刷新间隔
./build/schedtop -i 2

# 显示更多进程
./build/schedtop -n 20

# 过滤特定PID
./build/schedtop -p 1234

# 启动时显示直方图
./build/schedtop -H
```

### schedtop界面说明

启动schedtop后，你会看到类似以下的界面：

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                     Linux Kernel Schedule Monitor v1.0                       ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  Time: 2024-01-15 10:30:00  Refresh: 1s  PID Filter: 0                        ║
╚══════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────┐
│                           Kernel Statistics                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│  Total Events: 1,234,567  Dropped: 123  Anomalies: 456                      │
│  Max Delay: 234.567ms    Avg Delay: 1.234ms                                 │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│   Top 10 Processes by Total Delay                                            │
├───────┬──────────────────┬─────────────┬─────────────┬──────────┬─────────┤
│  PID  │       COMM       │ TOTAL DELAY │  MAX DELAY  │ SWITCHES │ ANOMALY │
├───────┼──────────────────┼─────────────┼─────────────┼──────────┼─────────┤
│  1234 │ firefox          │  1.234s     │   234.567ms │  123456  │    123  │
│  5678 │ chrome           │  987.654ms  │   123.456ms │   98765  │     45  │
│  9012 │ systemd          │  654.321ms  │    98.765ms │   65432  │     12  │
│  3456 │ dockerd          │  543.210ms  │    87.654ms │   54321  │      8  │
│  7890 │ kworker          │  432.109ms  │    76.543ms │   43210  │      5  │
└───────┴──────────────────┴─────────────┴─────────────┴──────────┴─────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              Commands                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│  q - Quit              h - Toggle histogram                                 │
│  p - Set PID filter    r - Set refresh interval                             │
│  c - Clear stats       s - Send config to kernel                            │
│  ? - Show this help                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 交互式命令

在schedtop运行时，可以使用以下命令：

| 命令 | 功能 | 示例 |
|------|------|------|
| `q` | 退出程序 | - |
| `h` | 切换直方图显示 | - |
| `p` | 设置PID过滤器 | 输入PID后按回车 |
| `r` | 设置刷新间隔 | 输入秒数后按回车 |
| `c` | 清除统计信息 | - |
| `s` | 发送配置到内核 | - |
| `?` | 显示帮助信息 | - |

### 实际操作示例

#### 示例1：监控特定进程

```bash
# 1. 找到目标进程PID
$ ps aux | grep firefox
user  1234  5.0  8.2  2345678 123456 ?  Sl  10:00   2:34 /usr/lib/firefox/firefox

# 2. 启动schedtop并过滤该PID
$ ./build/schedtop -p 1234

# 3. 在schedtop中按 'h' 查看延迟分布直方图
```

#### 示例2：压力测试监控

```bash
# 终端1：启动监控
$ ./build/schedtop -i 1 -H

# 终端2：运行压力测试
$ stress-ng --cpu 4 --timeout 60s

# 终端3：查看内核日志
$ watch -n 1 'dmesg | tail -20'
```

#### 示例3：异常检测

```bash
# 1. 加载模块时设置较低的异常阈值
sudo insmod kernel/sched_monitor.ko threshold_ns=50000000  # 50ms

# 2. 启动监控
./build/schedtop

# 3. 观察异常计数增加
# 当进程延迟超过50ms时，ANOMALY列会增加
```

#### 示例4：长期监控

```bash
# 1. 启动守护进程
./build/monitord -d -l /var/log/monitord.log -p /var/run/monitord.pid

# 2. 定期运行schedtop并保存结果
while true; do
    ./build/schedtop -n 20 > /tmp/schedtop_$(date +%Y%m%d_%H%M%S).txt
    sleep 300  # 每5分钟记录一次
done

# 3. 分析日志
grep "Total Events" /tmp/schedtop_*.txt
```

### 模块参数配置

#### 加载时配置

```bash
# 缓冲区大小（每个CPU的事件数）
sudo insmod sched_monitor.ko buffer_size=8192

# Workqueue执行间隔（毫秒）
sudo insmod sched_monitor.ko interval_ms=5

# 异常检测阈值（纳秒）
sudo insmod sched_monitor.ko threshold_ns=100000000  # 100ms

# PID过滤器（0表示不过滤）
sudo insmod sched_monitor.ko filter_pid=1234

# 启用/禁用监控
sudo insmod sched_monitor.ko enabled=1
```

#### 运行时配置

```bash
# 查看当前参数
$ cat /sys/module/sched_monitor/parameters/*
4096
10
100000000
0
Y

# 修改参数（需要root权限）
sudo sh -c 'echo 8192 > /sys/module/sched_monitor/parameters/buffer_size'
sudo sh -c 'echo 5 > /sys/module/sched_monitor/parameters/interval_ms'
```

### 卸载和清理

```bash
# 1. 停止schedtop（按q）

# 2. 停止monitord
kill $(cat /tmp/monitord.pid)

# 3. 卸载内核模块
sudo rmmod sched_monitor

# 4. 检查卸载日志
$ dmesg | tail -5
[ 2345.678901] sched_monitor: Unloading sched_monitor
[ 2345.678902] sched_monitor: Monitoring stopped
[ 2345.678903] sched_monitor: Final statistics:
[ 2345.678904] sched_monitor:   Total events: 1234567
[ 2345.678905] sched_monitor: Module unloaded successfully
```

## 项目架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         用户空间 (User Space)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────┐         ┌──────────────┐         ┌──────────────┐        │
│  │   schedtop   │         │   monitord   │         │   其他工具    │        │
│  │  (交互式监控) │         │  (守护进程)   │         │  (数据分析)   │        │
│  └──────┬───────┘         └──────┬───────┘         └──────┬───────┘        │
│         │                        │                        │                │
│         └────────────────────────┼────────────────────────┘                │
│                                  │                                         │
│                          ┌───────▼────────┐                                │
│                          │ stats_processor│                                │
│                          │  (统计处理)     │                                │
│                          └───────┬────────┘                                │
│                                  │                                         │
│                          ┌───────▼────────┐                                │
│                          │ netlink_client│                                │
│                          │ (Netlink客户端)│                                │
│                          └───────┬────────┘                                │
└──────────────────────────────────┼─────────────────────────────────────────┘
                                   │
                    ┌──────────────┼──────────────┐
                    │              │              │
           Netlink Socket (协议31)  │              │
                    │              │              │
└───────────────────┼──────────────┼──────────────┼───────────────────────┘
                    │              │              │
┌───────────────────▼──────────────▼──────────────▼───────────────────────┐
│                          内核空间 (Kernel Space)                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          ┌─────────────────┐                                │
│                          │  netlink_comm   │                                │
│                          │ (Netlink服务端)  │                                │
│                          └────────┬────────┘                                │
│                                   │                                         │
│                          ┌────────▼────────┐                                │
│                          │  async_worker   │                                │
│                          │  (异步工作队列)  │                                │
│                          └────────┬────────┘                                │
│                                   │                                         │
│              ┌────────────────────┼────────────────────┐                   │
│              │                    │                    │                   │
│      ┌───────▼────────┐   ┌──────▼───────┐   ┌───────▼────────┐           │
│      │  ring_buffer   │   │    stats     │   │    config      │           │
│      │ (Per-CPU缓冲区) │   │   (统计模块)  │   │   (配置管理)    │           │
│      └───────┬────────┘   └──────┬───────┘   └───────┬────────┘           │
│              │                   │                   │                   │
│              └───────────────────┼───────────────────┘                   │
│                                  │                                         │
│                          ┌───────▼────────┐                                │
│                          │  trace_hook    │                                │
│                          │ (Tracepoint钩子)│                                │
│                          └───────┬────────┘                                │
│                                  │                                         │
│              ┌───────────────────┼───────────────────┐                   │
│              │                   │                   │                   │
│      ┌───────▼────────┐  ┌──────▼───────┐  ┌──────▼───────┐              │
│      │ sched_switch   │  │ sched_wakeup │  │ sched_process│              │
│      │  (调度切换)     │  │  (进程唤醒)   │  │   (进程管理)  │              │
│      └────────────────┘  └─────────────┘  └─────────────┘              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
                          ┌─────────────────┐
                          │  Linux Scheduler│
                          │   (调度器)       │
                          └─────────────────┘
```

### 数据流图

```
调度事件发生
     │
     ▼
┌─────────────┐
│ Tracepoint  │  (sched_switch, sched_wakeup等)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ trace_hook  │  捕获事件，填充sched_event结构
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ ring_buffer │  写入Per-CPU缓冲区（无锁）
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   stats     │  更新统计信息
└──────┬──────┘
       │
       ▼
┌─────────────┐
│async_worker │  定期从缓冲区读取事件
└──────┬──────┘
       │
       ▼
┌─────────────┐
│netlink_comm │  通过Netlink发送到用户空间
└──────┬──────┘
       │
       ▼
┌─────────────┐
│monitord/    │  接收事件，更新缓存
│schedtop     │  显示统计信息
└─────────────┘
```

### 目录结构

```
sched-monitor/
├── include/
│   └── sched_monitor.h          # 公共头文件（内核+用户态共享）
├── kernel/                       # 内核模块
│   ├── kernel_internal.h        # 内核内部头文件
│   ├── main.c                   # 模块入口
│   ├── trace_hook.c             # Tracepoint钩子实现
│   ├── ring_buffer.c            # Per-CPU环形缓冲区
│   ├── netlink_comm.c           # Netlink通信模块
│   ├── async_worker.c           # Workqueue异步处理
│   ├── config.c                 # 配置管理
│   ├── stats.c                  # 统计模块
│   ├── Kbuild                   # 内核构建配置
│   └── Makefile                 # 内核模块Makefile
├── user/
│   ├── common/
│   │   ├── user_common.h        # 用户态公共头文件
│   │   ├── netlink_client.c     # Netlink客户端
│   │   └── stats_processor.c    # 统计处理器
│   ├── monitord/
│   │   └── monitord.c           # 守护进程
│   └── schedtop/
│       └── schedtop.c           # 交互式监控工具
├── scripts/
│   └── test.sh                  # 测试脚本
├── CMakeLists.txt               # CMake配置
├── Makefile                     # 顶层Makefile
└── README.md                    # 项目文档
```

## 技术细节

### 1. Tracepoint机制

#### 原理

Tracepoint是Linux内核提供的一种动态跟踪机制，允许在内核代码的关键位置插入探针（probe），而无需修改内核源码。

#### 实现细节

```c
// 注册tracepoint探针
static void probe_sched_switch(void *ignore, bool preempt,
                               struct task_struct *prev,
                               struct task_struct *next,
                               unsigned int prev_state)
{
    // 捕获调度切换事件
    struct sched_event event;
    // 填充事件结构
    // 写入环形缓冲区
}

// 注册探针
tracepoint_probe_register("sched_switch", probe_sched_switch, NULL);
```

#### 性能考虑

- **快速路径优化**：探针函数必须尽可能轻量
- **避免阻塞**：不能在探针中调用可能睡眠的函数
- **原子操作**：使用原子操作避免锁竞争

### 2. Per-CPU环形缓冲区

#### 设计原理

```c
struct per_cpu_buffer {
    struct sched_event *data;   // 事件数据数组
    u32 size;                   // 缓冲区大小
    atomic_t head;              // 写入位置（原子操作）
    atomic_t tail;              // 读取位置
    atomic_t count;             // 当前事件数
    atomic64_t overrun;         // 溢出计数
    spinlock_t lock;            // 读取时的锁
};
```

#### 无锁写入算法

```c
int ring_buffer_write_event(struct sched_event *event, int cpu)
{
    struct per_cpu_buffer *buf = per_cpu_ptr(g_state.buffers, cpu);
    
    // 原子获取并递增head
    u32 head = atomic_fetch_add(1, &buf->head);
    
    // 计算环形位置
    u32 pos = head % buf->size;
    
    // 检查缓冲区是否满
    u32 tail = atomic_read(&buf->tail);
    if (head - tail >= buf->size) {
        atomic64_inc(&buf->overrun);
        return -ENOSPC;
    }
    
    // 写入事件
    memcpy(&buf->data[pos], event, sizeof(struct sched_event));
    
    return 0;
}
```

#### 优势

- **零锁竞争**：每个CPU独立缓冲区
- **缓存友好**：数据局部性好
- **低延迟**：原子操作比锁更快

### 3. Netlink通信

#### 协议设计

```c
// 自定义Netlink协议
#define SCHED_MONITOR_NETLINK      31
#define SCHED_MONITOR_GROUP        1

// 消息类型
enum sched_msg_type {
    SCHED_MSG_EVENT       = 1,    // 事件数据
    SCHED_MSG_CONFIG      = 2,    // 配置消息
    SCHED_MSG_STATS       = 3,    // 统计查询
    SCHED_MSG_ACK         = 4,    // 确认消息
};
```

#### 消息格式

```
+------------------+
| Netlink Header   |
+------------------+
| Message Header   |
+------------------+
| Payload          |
| - Event Count    |
| - Events[]       |
+------------------+
```

#### 多播支持

```c
// 内核端：发送多播消息
nlmsg_multicast(g_state.nl_sock, skb, 0, SCHED_MONITOR_GROUP, GFP_ATOMIC);

// 用户端：加入多播组
addr.nl_groups = SCHED_MONITOR_GROUP;
bind(sock, (struct sockaddr *)&addr, sizeof(addr));
```

### 4. 异步工作队列

#### 设计目的

将事件处理和Netlink传输移出调度路径，避免对调度性能的影响。

#### 实现细节

```c
// 创建专用工作队列
g_state.wq = alloc_workqueue("sched_monitor_wq",
                              WQ_UNBOUND | WQ_HIGHPRI,
                              1);

// 初始化延迟工作
INIT_DELAYED_WORK(&g_state.dwork, async_work_handler);

// 启动周期性工作
queue_delayed_work(g_state.wq, &g_state.dwork,
                   msecs_to_jiffies(interval_ms));
```

#### 工作流程

```
定时器触发
    │
    ▼
async_work_handler()
    │
    ├─> 从ring_buffer读取事件
    │
    ├─> 通过Netlink发送事件
    │
    └─> 定期发送统计信息
         │
         ▼
    重新调度（queue_delayed_work）
```

### 5. 统计与直方图

#### 直方图设计

```c
#define HISTOGRAM_BINS    16

static const u64 delay_bin_bounds[HISTOGRAM_BINS + 1] = {
    0,              // [0, 1us)
    1000,           // [1us, 10us)
    10000,          // [10us, 100us)
    100000,         // [100us, 1ms)
    1000000,        // [1ms, 10ms)
    10000000,       // [10ms, 100ms)
    100000000,      // [100ms, 1s)
    1000000000,     // [1s, 10s)
    10000000000ULL, // [10s, +inf)
    // ... 预留更细粒度的区间
};
```

#### 统计更新

```c
void stats_update(struct sched_event *event, int cpu)
{
    struct sched_stats *stats = this_cpu_ptr(g_state.per_cpu_stats);
    
    stats->total_events++;
    
    if (event->delay_ns > 0) {
        stats->total_delay_ns += event->delay_ns;
        
        // 更新最大/最小延迟
        if (event->delay_ns > stats->max_delay_ns)
            stats->max_delay_ns = event->delay_ns;
        if (event->delay_ns < stats->min_delay_ns)
            stats->min_delay_ns = event->delay_ns;
        
        // 更新直方图
        int bin = find_histogram_bin(event->delay_ns);
        stats->hist.bins[bin]++;
    }
}
```

### 6. 配置与过滤

#### PID过滤器

```c
bool config_check_filter(struct sched_event *event)
{
    if (g_state.config.flags & SCHED_FLAG_FILTER_PID) {
        // 只监控指定PID的进程
        if (event->prev_pid != g_state.config.filter_pid &&
            event->next_pid != g_state.config.filter_pid) {
            return false;
        }
    }
    return true;
}
```

#### 异常检测

```c
bool config_check_anomaly(struct sched_event *event)
{
    if (!(g_state.config.flags & SCHED_FLAG_ANOMALY_DETECT))
        return false;
    
    // 延迟超过阈值视为异常
    return event->delay_ns >= g_state.config.threshold_ns;
}
```

### 7. 内存管理

#### Per-CPU分配

```c
// 分配Per-CPU数据结构
g_state.buffers = alloc_percpu(struct per_cpu_buffer);
g_state.per_cpu_stats = alloc_percpu(struct sched_stats);

// 访问Per-CPU数据
struct per_cpu_buffer *buf = per_cpu_ptr(g_state.buffers, cpu);
struct sched_stats *stats = this_cpu_ptr(g_state.per_cpu_stats);
```

#### 页分配

```c
// 计算分配阶数
int order = get_order(size * sizeof(struct sched_event));

// 分配连续页
struct page *page = alloc_pages(GFP_KERNEL | __GFP_ZERO, order);

// 获取虚拟地址
void *data = page_address(page);
```

### 8. 并发控制

#### 原子操作

```c
// 原子递增
atomic_inc(&buf->count);

// 原子获取并递增
u32 head = atomic_fetch_add(1, &buf->head);

// 原子读取
u32 tail = atomic_read(&buf->tail);
```

#### 自旋锁

```c
// 初始化自旋锁
spin_lock_init(&buf->lock);

// 加锁（保存中断状态）
unsigned long flags;
spin_lock_irqsave(&buf->lock, flags);

// 解锁
spin_unlock_irqrestore(&buf->lock, flags);
```

#### 互斥锁

```c
// 初始化互斥锁
mutex_init(&g_state.config_lock);

// 加锁
mutex_lock(&g_state.config_lock);

// 解锁
mutex_unlock(&g_state.config_lock);
```

### 9. 性能优化技巧

#### 缓存行对齐

```c
struct sched_event {
    __u64 timestamp;
    __u32 prev_pid;
    // ...
} __attribute__((packed, aligned(64)));  // 64字节对齐，避免false sharing
```

#### 批量处理

```c
// 批量读取事件
int ring_buffer_read_events(struct sched_event *events, int max_count,
                            int *cpu_map, int *count_map)
{
    // 一次读取最多MAX_EVENTS_PER_MSG个事件
    events_per_cpu = min_t(int, count, MAX_EVENTS_PER_MSG);
}
```

#### 内存预分配

```c
// 在初始化时分配所有内存
static void *event_buffer;
event_buffer = kmalloc_array(MAX_EVENTS_PER_MSG, 
                              sizeof(struct sched_event),
                              GFP_KERNEL);
```

## 常见问题解答

### 编译相关

**Q1: 编译时提示找不到内核头文件**

```
错误：make: *** /lib/modules/5.15.0-91-generic/build: No such file or directory
```

**A:** 安装内核头文件
```bash
sudo apt install linux-headers-$(uname -r)
```

**Q2: 编译时提示权限不足**

```
错误：make: cannot create directory '/lib/modules/...': Permission denied
```

**A:** 使用sudo编译内核模块
```bash
sudo make -C kernel
```

**Q3: CMake找不到编译器**

```
错误：No CMAKE_C_COMPILER could be found
```

**A:** 安装build-essential
```bash
sudo apt install build-essential
```

### 运行相关

**Q4: 加载模块时提示"Invalid module format"**

```
错误：insmod: ERROR: could not insert module: Invalid module format
```

**A:** 检查内核版本是否匹配
```bash
# 查看模块版本
modinfo kernel/sched_monitor.ko | grep vermagic

# 查看当前内核版本
uname -r

# 重新编译
make clean
make kernel
```

**Q5: 模块加载后dmesg没有输出**

**A:** 检查内核日志级别
```bash
# 查看当前日志级别
cat /proc/sys/kernel/printk

# 提高日志级别
sudo sh -c 'echo 8 > /proc/sys/kernel/printk'

# 或者使用dmesg -n 8
```

**Q6: schedtop启动失败，提示"Failed to initialize Netlink client"**

**A:** 检查内核模块是否加载
```bash
# 检查模块是否加载
lsmod | grep sched_monitor

# 如果没有加载，先加载模块
sudo insmod kernel/sched_monitor.ko
```

**Q7: schedtop显示"No events received"**

**A:** 检查监控是否启用
```bash
# 检查监控状态
cat /sys/module/sched_monitor/parameters/enabled

# 如果是N，启用监控
sudo sh -c 'echo Y > /sys/module/sched_monitor/parameters/enabled'

# 或者重新加载模块
sudo rmmod sched_monitor
sudo insmod kernel/sched_monitor.ko enabled=1
```

### 性能相关

**Q8: 监控导致系统性能下降**

**A:** 调整参数减少开销
```bash
# 增大缓冲区，减少Netlink传输频率
sudo insmod sched_monitor.ko buffer_size=8192 interval_ms=50

# 或者禁用监控
sudo sh -c 'echo N > /sys/module/sched_monitor/parameters/enabled'
```

**Q9: Dropped Events持续增加**

**A:** 增大缓冲区或减少监控频率
```bash
# 增大缓冲区
sudo sh -c 'echo 8192 > /sys/module/sched_monitor/parameters/buffer_size'

# 减少监控频率
sudo sh -c 'echo 50 > /sys/module/sched_monitor/parameters/interval_ms'
```

**Q10: 内存占用过高**

**A:** 检查Per-CPU缓冲区大小
```bash
# 计算内存占用
# 内存 = CPU数量 × buffer_size × sizeof(sched_event)
# 例如：8 CPU × 8192 × 128字节 = 8MB

# 减小缓冲区
sudo sh -c 'echo 2048 > /sys/module/sched_monitor/parameters/buffer_size'
```

### 调试相关

**Q11: 如何查看详细的内核日志？**

**A:** 使用dmesg和动态调试
```bash
# 查看最近的内核日志
dmesg | tail -50

# 实时监控内核日志
dmesg -w

# 过滤sched_monitor相关日志
dmesg | grep sched_monitor
```

**Q12: 如何调试Netlink通信问题？**

**A:** 使用tcpdump或strace
```bash
# 监控Netlink通信（需要root）
sudo tcpdump -i any -nn 'netlink'

# 跟踪系统调用
strace -e trace=socket,bind,sendmsg,recvmsg ./build/schedtop
```

**Q13: 如何分析性能瓶颈？**

**A:** 使用perf工具
```bash
# 分析内核模块性能
sudo perf top -g

# 分析特定函数
sudo perf record -e sched:* -a
sudo perf report
```

### 其他问题

**Q14: 如何在系统启动时自动加载模块？**

**A:** 创建模块加载配置
```bash
# 创建配置文件
sudo nano /etc/modules-load.d/sched_monitor.conf

# 添加内容
sched_monitor

# 创建参数配置
sudo nano /etc/modprobe.d/sched_monitor.conf

# 添加内容
options sched_monitor buffer_size=4096 interval_ms=10 threshold_ns=100000000
```

**Q15: 如何卸载模块时提示"Module is in use"？**

**A:** 停止使用模块的程序
```bash
# 停止monitord
killall monitord

# 停止schedtop
killall schedtop

# 然后卸载
sudo rmmod sched_monitor
```

**Q16: 如何查看模块的统计信息？**

**A:** 使用schedtop或查询内核统计
```bash
# 方法1：使用schedtop
./build/schedtop

# 方法2：查看内核统计（需要扩展sysfs接口）
# 当前版本需要通过Netlink查询
```

**Q17: 如何在不同内核版本间移植？**

**A:** 检查API兼容性
```bash
# 查看内核版本差异
uname -r

# 检查tracepoint API是否可用
grep -r "tracepoint_probe_register" /lib/modules/$(uname -r)/build/include/

# 可能需要修改代码以适配不同版本
```

**Q18: 如何在虚拟机中测试？**

**A:** 确保虚拟机支持内核模块加载
```bash
# 检查内核模块支持
ls /proc/modules

# 如果是容器，需要特权模式
docker run --privileged ...
```

## 面试相关

### 核心概念

#### 1. 什么是Tracepoint？

**答案：**

Tracepoint是Linux内核提供的一种动态跟踪机制，允许在内核代码的关键位置插入探针（probe），用于捕获和分析内核事件。

**特点：**
- 零侵入性：无需修改内核源码
- 低开销：探针未注册时几乎无性能影响
- 灵活性：可以动态注册/注销探针
- 上下文信息：提供丰富的内核上下文

**应用场景：**
- 性能分析
- 调试
- 监控
- 安全审计

#### 2. 为什么使用Per-CPU数据结构？

**答案：**

Per-CPU数据结构为每个CPU分配独立的副本，具有以下优势：

**优势：**
1. **避免锁竞争**：每个CPU独立访问，无需同步
2. **缓存友好**：数据局部性好，减少缓存失效
3. **低延迟**：原子操作比锁更快
4. **可扩展性**：随CPU数量线性扩展

**实现方式：**
```c
// 分配
struct per_cpu_buffer *buffers = alloc_percpu(struct per_cpu_buffer);

// 访问
struct per_cpu_buffer *buf = per_cpu_ptr(buffers, cpu);
struct per_cpu_buffer *buf = this_cpu_ptr(buffers);
```

#### 3. 什么是Netlink？为什么使用Netlink而不是其他IPC机制？

**答案：**

Netlink是Linux特有的用户态-内核态通信机制。

**Netlink特点：**
- 专门为内核-用户态通信设计
- 支持异步通信
- 支持多播（一对多）
- 支持消息序列号
- 内置在内核中，无需额外依赖

**对比其他IPC：**

| 机制 | 优点 | 缺点 |
|------|------|------|
| Netlink | 专为内核设计，支持多播 | 仅Linux支持 |
| ioctl | 简单易用 | 同步，性能差 |
| procfs/sysfs | 易于访问 | 不适合大数据传输 |
| 共享内存 | 性能最高 | 同步复杂 |

#### 4. 什么是环形缓冲区？如何实现无锁环形缓冲区？

**答案：**

环形缓冲区（Ring Buffer）是一种先进先出（FIFO）的数据结构，使用固定大小的数组循环利用空间。

**无锁实现原理：**
1. **原子操作**：使用原子变量维护head和tail指针
2. **CAS操作**：使用compare-and-swap确保原子性
3. **内存屏障**：防止指令重排序

**代码示例：**
```c
u32 head = atomic_fetch_add(1, &buf->head);
u32 pos = head % buf->size;
memcpy(&buf->data[pos], event, sizeof(*event));
```

**注意事项：**
- 写入方无锁，读取方需要锁
- 需要处理缓冲区满的情况
- 使用内存屏障确保可见性

### 系统设计

#### 5. 如何设计一个高性能的内核监控系统？

**答案：**

**设计原则：**
1. **最小化侵入性**：使用tracepoint而非修改内核
2. **异步处理**：将处理移出关键路径
3. **批量传输**：减少通信开销
4. **Per-CPU设计**：避免锁竞争
5. **内存预分配**：避免运行时分配

**架构设计：**
```
调度路径（快速路径）
  ↓
Tracepoint探针（最小化处理）
  ↓
Per-CPU环形缓冲区（无锁写入）
  ↓
异步工作队列（批量处理）
  ↓
Netlink传输（用户空间）
```

#### 6. 如何处理高并发场景下的数据竞争？

**答案：**

**策略：**
1. **Per-CPU数据**：每个CPU独立副本
2. **原子操作**：简单计数器使用atomic_t
3. **RCU**：读多写少场景
4. **自旋锁**：短临界区
5. **互斥锁**：长临界区，可睡眠

**示例：**
```c
// 写入：原子操作
atomic_inc(&stats->total_events);

// 读取：RCU
rcu_read_lock();
struct data *p = rcu_dereference(global_data);
rcu_read_unlock();

// 批量操作：自旋锁
spin_lock_irqsave(&lock, flags);
// 批量处理
spin_unlock_irqrestore(&lock, flags);
```

#### 7. 如何优化内核模块的性能？

**答案：**

**优化技巧：**
1. **减少内存分配**：使用静态分配或内存池
2. **批量处理**：减少函数调用开销
3. **缓存行对齐**：避免false sharing
4. **避免锁**：使用无锁数据结构
5. **减少上下文切换**：使用工作队列
6. **使用位操作**：替代除法/取模

**代码示例：**
```c
// 缓存行对齐
struct data {
    int counter;
} __attribute__((aligned(64)));

// 位操作替代取模
u32 pos = head & (size - 1);  // size是2的幂
```

### 性能分析

#### 8. 如何分析调度延迟？

**答案：**

**调度延迟定义：**
- 从进程就绪到实际运行的时间
- 包括：等待时间 + 抢占延迟

**分析方法：**
1. **捕获调度事件**：使用tracepoint
2. **计算时间差**：记录时间戳
3. **统计分析**：计算平均值、最大值、分布

**关键指标：**
- 平均延迟
- 最大延迟（P99, P99.9）
- 延迟分布（直方图）
- 异常事件计数

#### 9. 如何检测调度异常？

**答案：**

**异常类型：**
1. **延迟过大**：超过阈值
2. **频繁切换**：切换次数异常
3. **饥饿**：进程长时间未运行

**检测方法：**
```c
// 阈值检测
if (delay_ns > threshold_ns) {
    anomaly_count++;
}

// 统计检测
if (switch_count > expected_rate) {
    // 可能是抖动
}
```

### 实战问题

#### 10. 如何调试内核模块崩溃？

**答案：**

**调试步骤：**
1. **分析kdump**：使用crash工具分析内核转储
2. **查看日志**：dmesg, /var/log/kern.log
3. **使用ftrace**：跟踪函数调用
4. **使用kprobes**：动态插入探针
5. **代码审查**：检查内存访问、锁使用

**常用命令：**
```bash
# 查看内核日志
dmesg | tail -100

# 启用ftrace
echo 1 > /sys/kernel/debug/tracing/events/sched/enable

# 使用crash分析kdump
crash /usr/lib/debug/lib/modules/$(uname -r)/vmlinux /var/crash/vmcore
```

#### 11. 如何处理内核模块的版本兼容性？

**答案：**

**兼容性策略：**
1. **使用条件编译**：根据内核版本选择API
2. **检查内核版本**：运行时检查
3. **使用内核导出符号**：避免使用内部API
4. **提供兼容层**：封装不同版本的API

**代码示例：**
```c
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
    // 新API
#else
    // 旧API
#endif
```

#### 12. 如何设计可扩展的监控系统架构？

**答案：**

**设计原则：**
1. **模块化**：功能解耦
2. **插件化**：支持动态扩展
3. **配置化**：运行时可配置
4. **接口标准化**：定义清晰的API

**架构示例：**
```
核心框架
  ├─ 插件接口
  ├─ 事件总线
  └─ 配置管理

插件
  ├─ tracepoint插件
  ├─ kprobe插件
  └─ eBPF插件

后端
  ├─ Netlink后端
  ├─ eBPF后端
  └─ 共享内存后端
```

### 深入问题

#### 13. Tracepoint和Kprobe的区别是什么？

**答案：**

| 特性 | Tracepoint | Kprobe |
|------|-----------|--------|
| 位置 | 预定义位置 | 任意函数/指令 |
| 性能 | 更高 | 较低 |
| 稳定性 | 稳定 | 可能随内核变化 |
| 上下文 | 提供丰富上下文 | 有限 |
| 使用场景 | 调试、监控 | 调试、性能分析 |

**选择建议：**
- 使用Tracepoint：监控已知事件
- 使用Kprobe：调试特定函数

#### 14. 如何实现零拷贝的数据传输？

**答案：**

**零拷贝技术：**
1. **共享内存**：内核和用户态共享内存区域
2. **mmap**：将内核内存映射到用户空间
3. **splice**：在文件描述符间传输数据

**实现示例：**
```c
// 内核端
void *shared_buf = vmalloc_user(size);

// 用户端
void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
```

**注意事项：**
- 需要同步机制
- 内存管理复杂
- 安全性考虑

#### 15. 如何监控系统的实时性能？

**答案：**

**实时性能指标：**
1. **调度延迟**：进程等待时间
2. **中断延迟**：中断响应时间
3. **上下文切换率**：切换频率
4. **CPU利用率**：各CPU负载

**监控方法：**
1. **tracepoint**：捕获调度事件
2. **perf**：性能计数器
3. **/proc/stat**：系统统计
4. **/proc/interrupts**：中断统计

**分析工具：**
- perf top
- perf record/report
- trace-cmd
- bpftrace

## 开发指南

### 添加新的Tracepoint

1. 在`trace_hook.c`中添加探针函数
2. 在`trace_probes`数组中注册
3. 测试验证

### 扩展统计功能

1. 在`sched_stats`结构中添加字段
2. 在`stats.c`中实现更新逻辑
3. 在用户态工具中显示

### 自定义监控策略

1. 在`config.c`中添加配置项
2. 在`trace_hook.c`中实现过滤逻辑
3. 通过Netlink动态配置

### 性能调优建议

1. 根据CPU数量调整buffer_size
2. 根据负载调整interval_ms
3. 根据需求设置threshold_ns
4. 使用Per-CPU数据结构
5. 避免在探针中调用复杂函数

## 许可证

GPL v2

## 贡献指南

欢迎提交Issue和Pull Request！

## 联系方式

- 作者：Your Name
- 邮箱：your.email@example.com
- 项目地址：https://github.com/yourusername/sched-monitor
```