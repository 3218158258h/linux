
### 环境准备
#### 1. 交叉编译工具链
嵌入式程序通常在 PC 端用交叉编译工具链编译，需确保工具链中包含 **交叉 GDB 工具**（如 `arm-linux-gnueabihf-gdb`），以及目标板上可运行的 **GDB 服务器**（`gdbserver`）。


需要在开发主机上安装**交叉编译工具链**，其中包含针对目标架构的 GDB（如 `arm-linux-gnueabihf-gdb`）。

- **安装方式**：
  - 通过包管理器（如 Ubuntu/Debian）：
    ```bash
    # 以 ARM 架构为例
    sudo apt-get install gcc-arm-linux-gnueabihf gdb-multiarch
    ```
  - 或从芯片厂商官网下载专用工具链（如 Linaro、TI 等），解压后添加到环境变量 `PATH` 中。

- **验证安装**：
  ```bash
  arm-linux-gnueabihf-gdb --version  # 查看交叉 GDB 版本
  ```



- 若工具链未自带 `gdbserver`，需单独交叉编译：
  ```bash
  # 以 arm 架构为例，下载 gdb 源码后配置交叉编译
  ./configure --target=arm-linux-gnueabihf --host=arm-linux-gnueabihf --prefix=/path/to/install
  make && make install
  ```
  编译后将生成的 `gdbserver` 拷贝到目标板的 `/usr/bin` 或其他可执行路径。

#### 2. 编译带调试信息的程序
编译目标程序时，需添加 `-g` 或 `-ggdb` 选项保留调试信息，且**不要使用 `-O` 优化**（优化可能导致代码执行顺序与源码不一致，影响调试）：
```bash
arm-linux-gnueabihf-gcc -g -O0 test.c -o test  # 交叉编译带调试信息的程序
gcc -g -o myprogram myprogram.c  # 编译C程序，生成带调试信息的myprogram
g++ -ggdb -o mycppprogram mycppprogram.cpp  # 编译C++程序
```


### 2. 核心调试命令
以下是最常用的gdb命令（可使用缩写，如`b`代替`break`）：

| 命令格式                | 功能描述                                                                 |
|-------------------------|--------------------------------------------------------------------------|
| `run [args]` 或 `r [args]` | 启动程序（可带命令行参数），如 `r input.txt`                             |
| `break <位置>` 或 `b <位置>` | 设置断点：<br>- 按行号：`b 100`（在第100行设断点）<br>- 按函数名：`b main`（在main函数入口设断点）<br>- 按文件名+行号：`b test.c:50` |
| `next` 或 `n`           | 单步执行（跳过函数调用，将函数视为一个整体执行）                         |
| `step` 或 `s`           | 单步执行（进入函数内部，逐行调试函数内代码）                             |
| `continue` 或 `c`       | 从当前断点继续执行，直到遇到下一个断点或程序结束                         |
| `print <变量>` 或 `p <变量>` | 打印变量值，如 `p count`、`p *ptr`（打印指针指向的内容）                  |
| `backtrace` 或 `bt`     | 查看当前调用栈（函数调用关系），用于定位程序崩溃位置                     |
| `frame <n>` 或 `f <n>`  | 切换到调用栈的第n帧（配合`bt`使用，查看不同函数的上下文）                 |
| `watch <变量>`          | 设置监视点：当变量值被修改时暂停程序                                     |
| `delete <断点号>` 或 `d <断点号>` | 删除指定断点（用`info breakpoints`查看断点号）                           |
| `quit` 或 `q`           | 退出gdb                                                                  |


### 三、常用调试方式
#### 1. 直接在目标板上调试（本地调试）
若目标板资源充足（有终端、存储空间足够），可直接在目标板上运行 `gdb`：
```bash
# 在目标板终端执行
gdb ./test  # 启动 gdb 并加载程序
(gdb) run    # 运行程序
(gdb) break main.c:10  # 在 main.c 第10行设置断点
(gdb) step   # 单步执行（进入函数）
(gdb) next   # 单步执行（不进入函数）
(gdb) print var  # 打印变量值
(gdb) backtrace  # 查看函数调用栈
(gdb) quit   # 退出 gdb
```

#### 2. 远程调试（推荐）
适用于目标板资源有限（无 `gdb` 或终端交互不便），通过网络或串口在 PC 端控制调试：

##### 步骤1：在目标板上启动 `gdbserver`
```bash
# 目标板终端：用 gdbserver 监听端口（如 1234），并指定要调试的程序
gdbserver :1234 ./test
# 输出类似：Process ./test created; pid = 1234
# 输出：Listening on port 1234
```
`gdbserver` 会等待 PC 端的 `gdb` 连接。

##### 步骤2：在 PC 端用交叉 GDB 连接目标板
```bash
# PC 端终端：启动交叉 GDB（工具链中的 gdb）
arm-linux-gnueabihf-gdb ./test  # 加载本地编译的带调试信息的程序

# 在 gdb 交互界面中连接目标板
(gdb) target remote 192.168.1.100:1234  # 目标板 IP 和端口（与 gdbserver 对应）
# 连接成功后，即可像本地调试一样操作
(gdb) break main  # 设置断点
(gdb) continue    # 开始执行程序
(gdb) print i     # 查看变量 i 的值
(gdb) watch i     # 监视变量 i（值变化时暂停）
```



### 3. 实战示例
假设调试以下`test.c`程序（存在逻辑错误）：
```c
#include <stdio.h>

int sum(int a, int b) {
    return a + b;  // 假设此处有错误，应为a * b
}

int main() {
    int x = 3, y = 4;
    int result = sum(x, y);
    printf("结果: %d\n", result);  // 预期输出12，实际输出7
    return 0;
}
```

#### 调试步骤：
1. **编译带调试信息的程序**：
   ```bash
   gcc -g -o test test.c
   ```
2. **启动gdb并设置断点**：
   ```bash
   gdb ./test
   (gdb) b main    # 在main函数入口设断点
   (gdb) b sum     # 在sum函数入口设断点
   ```
3. **运行程序并单步调试**：
   ```bash
   (gdb) r         # 启动程序，停在main函数断点
   (gdb) n         # 执行下一行（int result = sum(x, y);）
   (gdb) s         # 进入sum函数内部
   (gdb) p a       # 打印a的值（3）
   (gdb) p b       # 打印b的值（4）
   (gdb) n         # 执行return a + b;（发现错误：应为乘法）
   (gdb) c         # 继续执行到程序结束
   ```


