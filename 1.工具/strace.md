## strace 核心用法
`strace` 的核心是**追踪程序发起的所有系统调用**，包括调用名、参数、返回值、错误码，下面结合程序演示最常用的用法：

### 1. 基础用法：追踪所有系统调用
```bash
strace ./strace_demo
```
执行后会输出大量日志，每一行对应一个系统调用，格式为：
`调用名(参数) = 返回值`
比如：
```
open("strace_demo.txt", O_RDWR|O_CREAT, 0644) = 3
write(3, "Hello strace!\n", 13)         = 13
```

### 2. 过滤指定系统调用（重点关注）
新手不需要看所有调用，用 `-e` 过滤关键调用（比如只看文件操作）：
```bash
# 只追踪 open/read/write/close 系统调用
strace -e trace=open,read,write,close ./strace_demo

# 只追踪网络相关系统调用（socket/connect）
strace -e trace=network ./strace_demo

# 只追踪错误的系统调用（快速定位问题）
strace -e trace=failed ./strace_demo
```
执行 `-e trace=failed` 会看到 `connect` 失败的详细原因：
```
connect(3, {sa_family=AF_INET, sin_port=htons(9999), sin_addr=inet_addr("0.0.0.0")}, 16) = -1 ECONNREFUSED (Connection refused)
```

### 3. 输出到文件（方便分析）
日志太多时，用 `-o` 保存到文件：
```bash
strace -o strace_log.txt ./strace_demo
# 查看日志
cat strace_log.txt
```

### 4. 显示时间戳（分析调用耗时）
用 `-t`/`-tt` 显示调用的时间，定位慢调用：
```bash
# -t 显示时分秒，-tt 显示到微秒
strace -tt ./strace_demo
```
输出示例：
```
15:30:45.123456 open("strace_demo.txt", O_RDWR|O_CREAT, 0644) = 3
```

### 5. 追踪已运行的进程（-p）
先启动程序，再用 `ps` 查进程 ID，然后追踪：
```bash
# 后台运行程序
./strace_demo &
# 查进程 ID
ps aux | grep strace_demo
# 追踪该进程（替换为实际 PID）
strace -p 12345
```

### 6. 显示调用耗时（-T）
查看每个系统调用的执行时间，定位慢调用：
```bash
strace -T ./strace_demo
```
输出示例（`0.000123` 是调用耗时）：
```
write(3, "Hello strace!\n", 13) = 13 <0.000123>
```
