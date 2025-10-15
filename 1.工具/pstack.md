`pstack` 是一个在 Unix/Linux 系统上用于打印进程中线程堆栈信息的命令行工具，类似于 Java 中的 `jstack`，但它适用于**所有类型的进程**（不仅限于 Java 程序），包括 C/C++ 等原生语言编写的应用。


### 主要功能
1. **查看进程内所有线程的堆栈跟踪**：显示进程中每个线程的函数调用链，包括正在执行的函数、调用来源等。
2. **分析进程状态**：帮助定位进程无响应、死锁、阻塞等问题，通过堆栈信息判断线程是否卡在某个系统调用或函数中。
3. **支持多线程程序**：能清晰展示多线程环境下各线程的执行状态和调用关系。


### 基本用法
```bash
pstack <进程ID>
```

- `<进程ID>`：目标进程的 PID（可通过 `ps` 或 `top` 命令获取）。
- 示例：
  ```bash
  # 先查看进程PID
  ps -ef | grep myprogram
  
  # 打印该进程的线程堆栈
  pstack 12345  # 12345 是目标进程的PID
  ```


### 输出信息解读
`pstack` 的输出通常包含：
- 线程 ID（LWP，Light Weight Process）。
- 线程状态（如 `R` 运行、`S` 睡眠、`D` 不可中断睡眠等）。
- 函数调用堆栈：从当前执行的函数开始，依次显示调用它的上级函数，直到进程启动入口（如 `main` 函数）。

例如，一段典型输出可能类似：
```
//线程2
Thread 2 (LWP 12346):
#0  0x00007f8a1b2a3a30 in pthread_cond_wait () from /lib64/libpthread.so.0    //当前执行的函数
#1  0x0000000000400a2b in worker_thread ()
#2  0x00007f8a1b29e734 in start_thread () from /lib64/libpthread.so.0
#3  0x00007f8a1b0215dd in clone () from /lib64/libc.so.6
- clone()（系统调用创建线程） → start_thread()（线程库启动逻辑） → worker_thread()（用户定义的线程函数） → pthread_cond_wait()（当前阻塞在此处）


//线程1（主线程）
Thread 1 (LWP 12345): 
#0  0x00007f8a1b016f0d in read () from /lib64/libc.so.6
#1  0x0000000000400b8c in main ()
```


### 典型应用场景
1. **排查进程无响应**：当程序卡住时，用 `pstack` 查看线程是否阻塞在某个系统调用（如 `read`、`write`）或锁操作（如 `pthread_mutex_lock`）上。
2. **分析死锁**：若多线程程序死锁，`pstack` 可显示线程正在等待的锁资源，帮助定位循环等待的线程。
3. **定位性能问题**：结合 `top -H` 找到 CPU 占用高的线程 ID，再用 `pstack` 查看该线程的堆栈，确定消耗资源的函数。


### 注意事项
- `pstack` 通常需要进程的调试权限（如 root 用户或进程所有者）才能正常使用。
- 部分系统可能默认不安装 `pstack`，可通过安装 `glibc-utils` 或 `procps` 包获取（如 `yum install glibc-utils` 或 `apt-get install procps`）。

总之，`pstack` 是 Linux 下调试多线程程序的重要工具，通过分析线程堆栈可快速定位进程的运行状态和潜在问题。