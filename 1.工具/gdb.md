1. 编译时添加 `-g` 选项启动GDB，添加`-O0`选项关闭优化，避免优化导致行号错位
```bash
gcc -g -O0 your_program.c -o your_program
```

2. 用 gdb 运行程序，定位段错误代码行

**方式1：直接运行程序，触发崩溃后调试**
```bash
gdb ./your_program  # 启动gdb，加载程序
(gdb) run           # 运行程序，触发段错误
```
程序崩溃后，gdb 会输出错误类型+崩溃的代码位置，此时输入 `bt`（backtrace）查看**调用栈**，就能找到崩溃的函数/代码行：
```bash
(gdb) bt  # 打印调用栈，核心命令！能看到崩溃时的函数调用路径
# 示例输出（能直接看到源码文件+行号）：
# #0  0x000055555555468a in main () at test.c:8  <-- 崩溃在test.c第8行
# #1  0x00007ffff7a05083 in __libc_start_main () from /lib64/libc.so.6
```

**方式2：程序已崩溃生成 core 文件（推荐，适合后台程序/已崩溃的场景）**
如果程序运行时崩溃，默认不会生成 core 文件（核心转储），需要先开启 core 文件生成：
```bash
# 1. 临时开启core文件（当前终端有效），设置core文件大小无限制
ulimit -c unlimited
# 2. 运行程序，崩溃后会生成 core.xxx（xxx是进程号）文件
./your_program
# 3. 用gdb加载core文件，直接分析崩溃现场（不用重新运行）
gdb ./your_program core.xxx
(gdb) bt  # 同样打印调用栈，定位代码行
```

3. gdb 常用调试命令

| 命令          | 作用                                                                 |
|---------------|----------------------------------------------------------------------|
| `run (r)`     | 运行程序，可传参数（比如 `run arg1 arg2`）                          |
| `backtrace (bt)` | 打印调用栈，`bt full` 会显示栈中变量的值（更详细）                  |
| `frame (f) n` | 切换到调用栈的第n帧（比如 `f 0` 切换到崩溃的函数帧）                |
| `print (p)`   | 打印变量值（比如 `p ptr` 看指针地址，`p *ptr` 看指针指向的内容）     |
| `list (l)`    | 显示当前行附近的源码（`l 行号` 显示指定行，`l 函数名` 显示函数）   |
| `info locals` | 显示当前栈帧的所有局部变量（找变量值异常，比如空指针）               |
| `break (b)`   | 设置断点（比如 `b test.c:8` 在第8行打断点，`b func` 在函数开头打断点） |
| `next (n)`    | 单步执行（不进入函数）                                              |
| `step (s)`    | 单步执行（进入函数）                                                |
| `continue (c)` | 继续执行   直到断点                                                        |
| `list`   | 列出源码，被截断时回车即可继续列出，list 1.30，列出30行                                                           |


```c
#include <stdio.h>
int main() {
    int *p = NULL;
    *p = 10;  // 第5行：空指针解引用，触发段错误
    return 0;
}
```
调试步骤：
```bash
gcc -g -O0 test.c -o test
gdb ./test
(gdb) run
# 输出：Program received signal SIGSEGV, Segmentation fault.
# 0x0000555555554665 in main () at test.c:5
# 5           *p = 10;
(gdb) bt  # 看调用栈，确认是main函数第5行
(gdb) p p  # 打印p的值，显示 $1 = (int *) 0x0（空指针），问题一目了然
```

