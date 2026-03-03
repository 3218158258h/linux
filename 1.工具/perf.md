`perf` 是 Linux 性能分析的核心工具，主打**CPU 热点定位、函数调用耗时分析、硬件事件（缓存/分支预测）追踪**

---

## 一、示例程序（perf_demo.c）
这个程序包含 CPU 密集型函数、多层函数调用、循环耗时操作，是练习 `perf` 的典型场景：

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 模拟小函数：计算平方（被高频调用）
int square(int num) {
    // 故意加空循环，增加CPU消耗
    for (int i = 0; i < 1000; i++);
    return num * num;
}

// 模拟中层函数：计算数组元素平方和
long long sum_squares(int *arr, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += square(arr[i]); // 高频调用 square 函数
    }
    return sum;
}

// 模拟CPU密集型主函数：生成随机数组并计算
void cpu_intensive_task() {
    int size = 100000;
    int *arr = (int*)malloc(size * sizeof(int));
    if (!arr) {
        perror("malloc failed");
        return;
    }

    // 初始化随机数组
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 1000;
    }

    // 多次调用 sum_squares，放大CPU消耗
    for (int i = 0; i < 100; i++) {
        long long result = sum_squares(arr, size);
        if (i % 10 == 0) { // 偶尔打印，避免IO干扰性能分析
            printf("Iteration %d: sum = %lld\n", i, result);
        }
    }
    free(arr);
}

int main() {
    printf("=== Start CPU intensive task ===\n");
    cpu_intensive_task(); // 核心耗时函数
    printf("=== Task finished ===\n");
    return 0;
}
```

### 编译程序（必须加调试信息！）
`perf` 分析函数名/行号需要调试信息，编译时加 `-g`，同时`-O0`关闭优化（避免编译器优化掉空循环）：
```bash
gcc -g -O0 -o perf_demo perf_demo.c
```

---

## 二、perf 核心用法
`perf` 的核心是**采样分析（perf record）** 和**报告查看（perf report）**，以下是最常用的功能，结合示例程序一步步操作：

### 1. 基础用法：CPU 热点分析（最常用）
目标：找到程序中占用 CPU 最多的函数（定位性能瓶颈）。

#### 步骤1：采样运行中的程序
```bash
# 方式1：直接启动程序并采样（采样10秒，-g 记录调用栈）
perf record -g ./perf_demo

# 方式2：先启动程序，再采样（适合长时间运行的程序）
./perf_demo &  # 后台运行程序
ps aux | grep perf_demo  # 查进程ID（比如 12345）
perf record -g -p 12345  # 采样该进程，按 Ctrl+C 停止采样
```
- `-g`：关键参数！记录函数调用栈，否则只能看到单个函数，看不到调用关系；
- 采样完成后会生成 `perf.data` 文件（采样数据）。

#### 步骤2：查看采样报告
```bash
perf report
```
进入交互界面后，你会看到：
- 第一列：`% overhead`（函数占用CPU的百分比）；
- 核心结果：`square` 函数的 overhead 最高（因为高频调用+空循环），其次是 `sum_squares`，最后是 `cpu_intensive_task`。

操作技巧：
- 按 `↑↓` 选择函数，按 `enter` 展开调用栈，能看到 `main → cpu_intensive_task → sum_squares → square` 的完整调用链；
- 按 `q` 退出报告。

### 2. 实时查看函数耗时（perf top）
目标：实时监控程序的 CPU 占用函数（类似 top，但精准到函数）。
```bash
# 实时显示CPU热点函数（-g 显示调用栈）
perf top -g -p 进程ID
```
你会看到 `square` 函数实时占据最高的 CPU 百分比，直观看到性能瓶颈。

### 3. 追踪系统调用（替代 strace）
目标：查看程序的系统调用（比 strace 效率更高）。
```bash
perf trace ./perf_demo
```
输出类似 `strace`，但更轻量化，能看到程序的 `malloc`、`free`、`write`（printf 底层）等系统调用。

### 4. 分析硬件事件（缓存/分支预测）
目标：深入硬件层面分析性能（进阶用法），比如查看缓存未命中、分支预测失败。
```bash
# 采样缓存未命中事件（-e 指定事件类型）
perf record -e cache-misses -g ./perf_demo
perf report  # 查看哪些函数导致缓存未命中

# 常用硬件事件：
# cache-misses：缓存未命中
# branch-misses：分支预测失败
# cpu-cycles：CPU周期
# instructions：指令数
```

### 5. 生成火焰图（直观展示调用栈）
火焰图是 `perf` 最实用的可视化功能，能一眼看到耗时最高的函数调用链（需安装火焰图工具）：
```bash
# 1. 安装火焰图工具（只需一次）
git clone https://github.com/brendangregg/FlameGraph.git
export PATH=$PATH:$(pwd)/FlameGraph

# 2. 采样并生成火焰图
perf record -g -F 99 ./perf_demo  # -F 99：每秒采样99次（避免频率过高）
perf script | stackcollapse-perf.pl > perf.out  # 转换采样数据
flamegraph.pl perf.out > perf_demo.svg  # 生成火焰图

# 3. 用浏览器打开 perf_demo.svg 查看
```
火焰图解读：
- 纵轴：函数调用栈（越往下是底层函数）；
- 横轴：CPU 耗时占比（越宽的函数，CPU 占用越高）；
- 你会看到 `square` 函数的柱子最宽，是性能瓶颈。

---

## 三、关键结果解读
通过 `perf` 分析这个程序，你会得到核心结论：
1. `square` 函数是 CPU 瓶颈（overhead 最高），因为高频调用+空循环；
2. 调用链：`main → cpu_intensive_task → sum_squares → square`；
3. 优化方向：删除 `square` 里的空循环，能大幅降低 CPU 消耗。

