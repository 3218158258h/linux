

## 一、示例程序（valgrind_demo.c）
这个程序包含 `valgrind` 能检测的所有典型内存问题，是练习的最佳样本：
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 错误1：内存泄漏（malloc后未free）
void memory_leak() {
    // 动态分配内存但不释放，valgrind会检测到"definitely lost"
    int *leak_ptr = (int*)malloc(10 * sizeof(int));
    leak_ptr[0] = 100; // 使用内存但不释放
    printf("Memory leak: allocated 10 ints, no free\n");
}

// 错误2：数组越界访问（堆内存越界）
void heap_out_of_bounds() {
    int *arr = (int*)malloc(5 * sizeof(int));
    if (!arr) return;

    // 合法下标 0-4，此处访问 arr[5] 越界
    arr[5] = 200; // valgrind会检测到"Invalid write of size 4"
    printf("Heap out of bounds: write arr[5] = %d\n", arr[5]);
    free(arr);
}

// 错误3：使用未初始化的变量
void uninitialized_var() {
    int uninit; // 未初始化
    // 使用未初始化变量，valgrind会检测到"Use of uninitialised value"
    if (uninit > 0) { // 此处读取未初始化的值
        printf("Uninitialized var: %d\n", uninit);
    }
}

// 错误4：双重释放（free两次同一块内存）
void double_free() {
    char *str = (char*)malloc(20);
    if (!str) return;

    strcpy(str, "double free test");
    free(str); // 第一次释放
    free(str); // 第二次释放，valgrind会检测到"Invalid free()"
    printf("Double free: freed same memory twice\n");
}

// 错误5：释放后使用（use-after-free）
void use_after_free() {
    int *ptr = (int*)malloc(sizeof(int));
    free(ptr); // 先释放
    *ptr = 300; // 释放后继续使用，valgrind会检测到"Invalid write of size 4"
    printf("Use after free: write to freed memory = %d\n", *ptr);
}

int main() {
    printf("=== Start valgrind demo (all memory errors) ===\n");
    
    memory_leak();
    heap_out_of_bounds();
    uninitialized_var();
    double_free(); // 执行到此处程序可能崩溃，可注释掉先测其他错误
    use_after_free(); // 若double_free崩溃，此行不会执行
    
    printf("=== End valgrind demo ===\n");
    return 0;
}
```

### 编译程序（必须加调试信息！）
`valgrind` 需要调试信息才能定位到具体代码行，编译时加 `-g` 且关闭优化：
```bash
gcc -g -O0 -o valgrind_demo valgrind_demo.c
```

---

## 二、valgrind 核心用法（手把手教学）
`valgrind` 核心工具是 `memcheck`（默认），专注检测内存错误，以下是最常用的命令：

### 1. 基础用法：检测所有内存错误
```bash
valgrind --leak-check=full ./valgrind_demo
```
- `--leak-check=full`：关键参数！不仅检测泄漏，还会显示泄漏的具体代码行；
- 执行后 `valgrind` 会输出详细的错误报告，逐条对应程序中的内存问题。

### 2. 关键输出解读（核心）
执行后会看到以下典型错误提示，对应程序中的问题：
| 错误类型               | valgrind 提示关键词                          | 对应函数          |
|------------------------|-----------------------------------------------|-------------------|
| 内存泄漏               | `definitely lost: 40 bytes in 1 block`        | memory_leak       |
| 堆内存越界             | `Invalid write of size 4` + 代码行号          | heap_out_of_bounds|
| 未初始化变量           | `Use of uninitialised value of size 4`        | uninitialized_var |
| 双重释放               | `Invalid free() / delete / delete[]`          | double_free       |
| 释放后使用             | `Invalid write of size 4` (freed memory)      | use_after_free    |

### 3. 进阶用法：输出详细日志（定位嵌入式问题）
嵌入式调试时，可将日志保存到文件，方便离线分析：
```bash
# 将错误日志保存到文件，同时显示详细的堆栈信息
valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_log.txt ./valgrind_demo

# 查看日志
cat valgrind_log.txt
```
- `--show-leak-kinds=all`：显示所有类型的内存泄漏（definitely lost/possibly lost等）；
- `--log-file`：将输出写入文件，适合嵌入式无终端的场景。

### 4. 过滤无关错误（嵌入式适配）
嵌入式程序可能调用系统库导致无关警告，可过滤掉：
```bash
# 忽略系统库的内存错误，只关注自己的代码
valgrind --leak-check=full --suppressions=/usr/lib/valgrind/default.supp ./valgrind_demo
```

### 5. 修复错误后验证
修改程序（比如给 `memory_leak` 加 `free(leak_ptr)`、注释掉 `double_free`），重新编译后执行：
```bash
valgrind --leak-check=full ./valgrind_demo
```
若输出 `All heap blocks were freed -- no leaks are possible`，说明内存问题已修复。

---

## 三、嵌入式场景适配要点
1. **交叉编译 valgrind**：
   嵌入式需为目标架构（如 ARM）交叉编译 valgrind，编译时指定：
   ```bash
   ./configure --target=arm-linux-gnueabihf --prefix=/opt/valgrind-arm
   make && make install
   ```
2. **内存开销注意**：
   valgrind 运行程序时内存开销会增加 2-10 倍，嵌入式低配设备（如 128MB 内存）需谨慎，建议在开发板上调试；
3. **只在调试阶段使用**：
   量产机删除 valgrind，避免性能损耗，仅在测试阶段用它排查内存问题。

---

## 四、valgrind 输出示例（关键片段）
以“内存泄漏”为例，valgrind 会精准定位到代码行：
```
==12345== 40 bytes in 1 block are definitely lost in loss record 1 of 5
==12345==    at 0x4848899: malloc (vg_replace_malloc.c:380)
==12345==    by 0x109162: memory_leak (valgrind_demo.c:9)
==12345==    by 0x109385: main (valgrind_demo.c:50)
```
解读：
- `40 bytes definitely lost`：确认泄漏 40 字节（10个int）；
- `by 0x109162: memory_leak (valgrind_demo.c:9)`：泄漏发生在 `valgrind_demo.c` 第9行的 `memory_leak` 函数。

