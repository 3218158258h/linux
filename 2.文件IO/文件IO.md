### 文件描述符
####
文件描述符是操作文件时系统随机分配的，每个文件不一样，
操作过程中要注意保存与关闭，避免浪费资源


`======================================================================`
### errno 
####
errno是一个全局整数变量（定义在 <errno.h>），用于存储系统调用或库函数的错误原因。
当函数返回 -1（或其他错误标识）时，errno 被设置为具体错误码，可通过 perror() 转换为可读信息。
#### 常见 `errno` 及场景
| 错误码       | 含义                  | 典型触发场景                                  |
|--------------|-----------------------|-----------------------------------------------|
| `ENOENT`     | 文件或目录不存在      | `open()` 打开不存在的文件且未用 `O_CREAT`      |
| `EACCES`     | 权限不足              | 访问无权限的文件，或目录缺少执行权限           |
| `EBADF`      | 无效的文件描述符      | `read()`/`close()` 传入未打开或已关闭的 FD     |
| `EEXIST`     | 文件已存在            | `open()` 用 `O_CREAT | O_EXCL` 但文件已存在    |
| `EMFILE`     | 进程打开 FD 数达上限  | 长期运行的服务未关闭 FD，导致超过 `RLIMIT_NOFILE` |
| `EAGAIN`/`EWOULDBLOCK` | 操作需阻塞（非阻塞模式） | `read()` 非阻塞 FD 且无数据，`write()` 缓冲区满 |
| `EIO`        | 输入输出错误          | 硬件故障、磁盘损坏等底层 IO 错误               |

`======================================================================`

`======================================================================`
## 一、常用文件IO系统函数
### 一、open函数原型
```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
```
- **返回值**：成功时返回一个非负整数（文件描述符）；失败时返回 `-1`，并设置 `errno` 表示错误原因。
- **参数**：
  - `pathname`：要打开或创建的文件路径（绝对路径或相对路径）。
  - `flags`：打开文件的方式和选项（必须包含以下一个访问模式）。
  - `mode`：仅当创建新文件时（`O_CREAT` 标志）有效，指定文件的权限（如 `0644`）。
##### 二、关键参数 `flags` 详解
`flags` 由**访问模式**和**额外选项**组合而成（用按位或 `|` 连接）。
###### 1. 必选访问模式（三选一）
- `O_RDONLY`：只读模式打开。
- `O_WRONLY`：只写模式打开。
- `O_RDWR`：可读可写模式打开。

###### 2. 常用额外选项（可组合，用|(或)分隔）
- `O_CREAT`：如果文件不存在，则创建它（需配合 `mode` 参数指定权限）。
- `O_EXCL`：与 `O_CREAT` 一起使用时，若文件已存在则报错（避免覆盖已有文件）。
- `O_TRUNC`：打开文件时清空原有内容（仅对已有文件有效）。
- `O_APPEND`：写入时追加到文件末尾（常用于日志文件）。
- `O_NONBLOCK`：非阻塞模式（对管道、设备文件有效，避免读写阻塞）。
##### 三、`mode` 参数（文件权限）
当使用 `O_CREAT` 时，`mode` 用于指定新文件的权限，通常用八进制数表示：
- `0644`：所有者可读可写（`6`），组用户可读（`4`），其他用户可读（`4`）。
- `0755`：所有者可读可写可执行（`7`），组用户和其他用户可读可执行（`5`）。
##### 四、示例代码
##### 1. 打开已有文件（只读）
```c
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int main() {
    int fd;
    // 只读模式打开文件
    fd = open("test.txt", O_RDONLY);
    if (fd == -1) {
        perror("打开文件失败");  // 输出错误信息（如文件不存在）
        return 1;
    }
    printf("文件打开成功，文件描述符：%d\n", fd);
    
    // 使用完毕后关闭文件
    close(fd);
    return 0;
}
```
#### 2. 创建并写入文件（清空原有内容）
```c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int fd;
    // 创建文件（若不存在），只写模式，打开时清空内容
    fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("创建文件失败");
        return 1;
    }
    
    // 写入内容
    const char *msg = "Hello, open()!\n";
    write(fd, msg, strlen(msg));  // 不包含字符串结束符 '\0'
    
    close(fd);
    return 0;
}
```

#### 3. 追加内容到文件
```c
// 以追加模式打开（在文件末尾写入）
fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
if (fd != -1) {
    write(fd, "新的日志内容\n", 12);
    close(fd);
}
```

`======================================================================`

`======================================================================`

### close函数原型与基础用法
```c
#include <unistd.h>
int close(int fd);
```
#### 参数与返回值：
- **`fd`**：需要关闭的文件描述符（如通过 `open`、`socket` 等函数获得的非负整数）。
- **返回值**：  
  - 成功：返回 `0`。  
  - 失败：返回 `-1`，并设置 `errno`（如 `EBADF` 表示 `fd` 无效或已关闭）。

`======================================================================`

`======================================================================`
### read函数原型与核心说明
```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
```
##### 参数解析：
- **`fd`**：文件描述符
- **`buf`**：指向接收数据的缓冲区
- **`count`**：计划读取的最大字节数（`size_t` 类型，非负）。
##### 返回值：
- **正数**：实际读取到的字节数（可能小于 `count`，因数据不足或被信号中断）。
- **0**：读到文件末尾（对常规文件）或对方关闭连接（对 Socket）。
- **-1**：读取失败，此时 `errno` 被设置为具体错误码（如 `EBADF` 表示无效 FD）。

`======================================================================`

`======================================================================`
### write函数原型与基础用法
```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);
```
##### 参数说明：
- **`fd`**：文件描述符（如通过 `open` 获得的文件句柄、标准输出 `1`、网络套接字等）。
- **`buf`**：指向待写入数据的缓冲区（需确保内存有效，且数据长度不小于 `count` 字节）。
- **`count`**：计划写入的字节数（`size_t` 类型，非负整数）。
##### 返回值：
- **正数**：实际写入的字节数（可能小于 `count`，非错误状态）。
- **0**：写入 0 字节（通常因 `count` 为 0）。
- **-1**：写入失败，此时 `errno` 被设置为具体错误码（如 `EBADF` 表示无效 FD）。

`======================================================================`

`======================================================================`

### `perror` 函数解析（自动打印error信息）
```c
#include <stdio.h>
void perror(const char *s);
```
#### 功能
- 打印由 `s` 指向的字符串（通常是对操作的描述），后跟一个冒号、空格，再后跟当前 `errno` 对应的错误信息（如 "No such file or directory"）。
1. **依赖 `errno`**：`perror` 的错误信息完全由全局变量 `errno` 的当前值决定，因此必须在函数返回错误（如 `open` 返回 `-1`）后立即调用，否则 `errno` 可能被其他函数修改。
2. **参数 `s` 的作用**：`s` 通常用于描述发生错误的操作（如 "open file"），使错误信息更具上下文，便于定位问题。若 `s` 为 `NULL` 或空字符串，则只打印错误信息。
```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
int main() {
    int fd = open("nonexistent.txt", O_RDONLY);
    if (fd == -1) {
        // 打印错误信息："Failed to open file: No such file or directory"
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return 0;
}
```
`perror` 是 C 标准库函数，用于打印最近一次系统调用（如 `open`）的错误信息，其参数需为**字符串**（用于提示错误场景），而非 `open` 函数名或 `fd`（文件描述符，整数），直接传 `perror(open)` 或 `perror(fd)` 是**错误用法**，会导致编译错误或逻辑错误。

#### 1. 正确用法：`perror("提示字符串")`
`perror` 的作用是：先打印参数中的“提示字符串 + `: `”，再打印与当前 `errno` 对应的错误描述（如“Permission denied”“No such file or directory”）。

`errno` 是系统维护的全局变量，当 `open`、`lseek` 等系统调用失败时，会自动设置 `errno` 为对应错误码，`perror` 正是通过读取 `errno` 来输出错误信息。
#### 2. 错误用法分析
| 错误写法       | 问题原因                                                                 |
|----------------|--------------------------------------------------------------------------|
| `perror(open)` | `open` 是函数名，传入后会被当作“函数地址”（整数），而非字符串，编译会报错。 |
| `perror(fd)`   | `fd` 是文件描述符（整数类型，如 `3`、`-1`），`perror` 要求参数是字符串，类型不匹配。 |
#### 3. 正确示例（结合 `open`）
```c
#include <stdio.h>   // perror 声明
#include <fcntl.h>   // open 声明
#include <unistd.h>  // close 声明
int main() {
    // 尝试打开一个不存在的文件，open 会失败并返回 -1，同时设置 errno
    int fd = open("nonexistent.txt", O_RDONLY);
    
    if (fd == -1) {  // 检查 open 是否失败
        // 正确用法：传字符串提示“open failed”，perror 会输出完整错误信息
        perror("open failed"); 
        return 1;  // 退出程序，避免后续错误
    }

    // 若 open 成功，后续操作...
    close(fd);
    return 0;
}
```
#### 输出结果（示例）：
```
open failed: No such file or directory
```
（“No such file or directory” 是 `errno` 对应的错误描述，由 `perror` 自动打印）

`======================================================================`

`======================================================================`
#### 3. 定位与属性
#### `lseek`基本用法
```c
#include <sys/types.h>
#include <unistd.h>
off_t lseek(int fd, off_t offset, int whence);
```
1. `fd`：文件描述符（通过 `open` 等函数获得）
2. `offset`：偏移量（根据 `whence` 解释其含义）
3. `whence`：偏移的基准位置，有三种取值：
   - `SEEK_SET`：从文件开头开始计算偏移
   - `SEEK_CUR`：从当前位置开始计算偏移
   - `SEEK_END`：从文件末尾开始计算偏移

#### 返回值
- 成功：返回新的文件偏移量（从文件开头算起的字节数）
- 失败：返回 `-1` 并设置 `errno`

```c
// 将文件指针移动到距离文件开头 100 字节处
off_t new_pos = lseek(fd, 100, SEEK_SET);

// 将文件指针从当前位置向后移动 50 字节
new_pos = lseek(fd, 50, SEEK_CUR);

// 将文件指针移动到文件末尾前 20 字节处
new_pos = lseek(fd, -20, SEEK_END);

// 获取当前文件指针位置（不改变位置）
off_t current_pos = lseek(fd, 0, SEEK_CUR);
```
`lseek` 只改变内核中维护的文件偏移量，不会实际读写文件数据。

`======================================================================`

`======================================================================`
#### `stat`基本用法
```c
#include <sys/stat.h>
int stat(const char *pathname, struct stat *statbuf);
```
1. `pathname`：文件的路径名（绝对路径或相对路径）
2. `statbuf`：指向`struct stat`结构体的指针，用于存储文件状态信息

#### 返回值
- 成功：返回`0`，文件状态信息已填充到`statbuf`中
- 失败：返回`-1`并设置`errno`

```c
struct stat file_info;
// 获取指定路径文件的状态信息
if (stat("/home/user/doc.txt", &file_info) == 0) {
    // 成功获取，可使用file_info中的信息
    printf("文件大小: %lld字节\n", (long long)file_info.st_size);
    printf("所有者ID: %d\n", file_info.st_uid);
} else {
    perror("stat失败");
}
```

#### `fstat`基本用法
```c
#include <sys/stat.h>
int fstat(int fd, struct stat *statbuf);
```
1. `fd`：已打开文件的文件描述符（通过`open`等函数获得）
2. `statbuf`：指向`struct stat`结构体的指针，用于存储文件状态信息

#### 返回值
- 成功：返回`0`，文件状态信息已填充到`statbuf`中
- 失败：返回`-1`并设置`errno`

```c
struct stat file_info;
int fd = open("data.bin", O_RDONLY);
if (fd != -1) {
    // 通过文件描述符获取文件状态信息
    if (fstat(fd, &file_info) == 0) {
        printf("文件权限: %o\n", file_info.st_mode & 0777);
        printf("最后修改时间: %ld\n", file_info.st_mtime);
    } else {
        perror("fstat失败");
    }
    close(fd);
}
```
`stat`和`fstat`获取的文件状态信息都存储在`struct stat`结构体中，包含文件大小、权限、所有者、时间戳等元数据。
`fstat`由于使用`已打开文件的描述符`，比`stat`更高效且更安全（避免文件被移动/删除的竞争条件）。

`======================================================================`

`======================================================================`
#### `dup`基本用法
```c
#include <unistd.h>
int dup(int oldfd);
```
1. `oldfd`：已打开的文件描述符，作为复制的源

#### 返回值
- 成功：返回一个新的文件描述符（最小的未使用文件描述符）
- 失败：返回`-1`并设置`errno`

```c
int fd = open("file.txt", O_RDWR);
if (fd != -1) {
    // 复制文件描述符
    int new_fd = dup(fd);
    if (new_fd != -1) {
        // 现在fd和new_fd指向同一个文件
        // 对其中一个进行读写会影响另一个的文件偏移量
        write(new_fd, "hello", 5);
        close(new_fd);  // 关闭新描述符
    } else {
        perror("dup失败");
    }
    close(fd);  // 关闭原始描述符
}
```
#### `dup2`基本用法
```c
#include <unistd.h>
int dup2(int oldfd, int newfd);
```
1. `oldfd`：已打开的文件描述符（源文件描述符）
2. `newfd`：指定要创建的新文件描述符编号

#### 返回值
- 成功：返回`newfd`（新的文件描述符）
- 失败：返回`-1`并设置`errno`

```c
int fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
if (fd != -1) {
    // 将stdout(1)重定向到文件output.txt
    if (dup2(fd, STDOUT_FILENO) == STDOUT_FILENO) {
        // 现在printf的输出会写入到文件而不是终端
        printf("这段文字会被写入文件\n");
        close(fd);  // 可以关闭原始fd，不影响重定向后的描述符
    } else {
        perror("dup2失败");
    }
}
```
`======================================================================`

`======================================================================`
#### `_exit`基本用法
```c
#include <unistd.h>
void _exit(int status);
```
1. `status`：程序的退出状态码，传递给父进程（与`exit`的`status`用法相同）
   - `0`通常表示正常退出
   - 非零值表示异常退出

#### 功能说明
- 立即终止当前进程，直接进入内核态
- 不执行任何用户态的清理操作：
  - 不会刷新文件流缓冲区（可能导致数据丢失）
  - 不会调用`atexit`注册的清理函数
  - 不会执行全局对象的析构函数（C++中）
- 会关闭所有打开的文件描述符，释放内核资源

```c
#include <unistd.h>
#include <stdio.h>

void cleanup() {
    printf("这个清理函数不会被执行\n");  // _exit不会调用
}

int main() {
    atexit(cleanup);  // 注册清理函数
    FILE *file = fopen("data.txt", "w");
    
    if (file != NULL) {
        fprintf(file, "这段内容可能不会被写入文件");  // 缓冲区未刷新
        _exit(0);  // 直接退出，不执行清理
    }
    
    _exit(1);
}
```

### 与`exit`的核心区别
| 特性 | `_exit` | `exit` |
|------|---------|--------|
| 清理操作 | 不执行用户态清理 | 执行用户态清理（刷新缓冲区、调用注册函数） |
| 适用场景 | 子进程退出、紧急退出 | 正常程序终止 |
| 标准 | POSIX系统调用 | C标准库函数 |

`======================================================================`

`======================================================================`
## 二、常用文件IO标准函数
#### `fopen`基本用法
```c
#include <stdio.h>
FILE *fopen(const char *pathname, const char *mode);
```
1. `pathname`：要打开的文件路径（绝对路径或相对路径）
2. `mode`：打开模式字符串，常用模式：
   - `"r"`：只读方式打开已有文件
   - `"w"`：写入方式打开，若文件存在则清空，不存在则创建
   - `"a"`：追加方式打开，新数据写入文件末尾
   - `"r+"`：读写方式打开已有文件
   - `"w+"`：读写方式打开，若文件存在则清空，不存在则创建
   - `"a+"`：读写方式打开，写入操作只能追加到末尾

#### 返回值
- 成功：返回指向`FILE`结构体的指针（文件流指针）
- 失败：返回`NULL`并设置`errno`

```c
// 以只读方式打开文本文件
FILE *file = fopen("data.txt", "r");//定义结构体
if (file == NULL) {
    perror("打开文件失败");
    return 1;
}
// 操作完成后必须关闭文件
fclose(file);
// 以写入方式创建二进制文件
FILE *bin_file = fopen("image.bin", "wb");
if (bin_file != NULL) {
    // 写入二进制数据...
    fclose(bin_file);
}
```
使用完毕后需用`fclose`关闭文件流，否则可能导致缓冲区数据未刷新而丢失。

`======================================================================`

`======================================================================`
#### `fclose`基本用法
```c
#include <stdio.h>
int fclose(FILE *stream);
```
1. `stream`：通过`fopen`等函数打开的文件流指针（`FILE*`类型）

#### 返回值
- 成功：返回`0`
- 失败：返回`EOF`（通常为`-1`）并设置`errno`

```c
FILE *file = fopen("notes.txt", "w");
if (file != NULL) {
    fprintf(file, "重要记录\n");
    // 关闭文件流
    if (fclose(file) != 0) {
        perror("关闭文件失败");
    } else {
        printf("文件已成功关闭\n");
    }
}
```

`======================================================================`

`======================================================================`
#### `fputs`基本用法
```c
#include <stdio.h>
int fputs(const char *str, FILE *stream);
```
1. `str`：指向要写入的以空字符`'\0'`结尾的字符串指针
2. `stream`：文件流指针（`FILE*`类型，通过`fopen`获得）

#### 返回值
- 成功：返回非负值（通常为非零值）
- 失败：返回`EOF`（通常为`-1`）并设置`errno`

```c
// 写入到文件
FILE *file = fopen("log.txt", "a");
if (file != NULL) {
    const char *message = "系统启动完成";
    
    if (fputs(message, file) != EOF) {
        fputs("\n", file);  // 手动添加换行符
        printf("消息已写入文件\n");
    } else {
        perror("写入文件失败");
    }
    fclose(file);
}
// 写入到标准输出（终端）
fputs("请输入用户名: ", stdout);
```
- 不会自动在字符串末尾添加换行符`'\n'`，需手动添加
- 字符串中的空字符`'\0'`是终止符，不会被写入
- 适合写入文本内容，不适合二进制数据

`======================================================================`

`======================================================================`
#### `fprintf`基本用法
```c
#include <stdio.h>
int fprintf(FILE *stream, const char *format, ...);
```
1. `stream`：文件流指针（`FILE*`类型，如`stdout`、`stderr`或通过`fopen`打开的文件）
2. `format`：格式控制字符串，包含普通字符和格式说明符（如`%d`、`%s`等）
3. `...`：可变参数列表，与格式说明符一一对应的值

#### 返回值
- 成功：返回写入的字符总数（不包括终止符`'\0'`）
- 失败：返回负数并设置`errno`

```c
// 写入到文件
FILE *log_file = fopen("app.log", "a");
if (log_file != NULL) {
    int user_id = 123;
    const char *action = "登录";
    
    // 格式化写入日志
    fprintf(log_file, "用户 %d 执行了 %s 操作\n", user_id, action);
    fclose(log_file);
}
// 写入到标准输出（终端）
int score = 95;
fprintf(stdout, "你的成绩是：%d分\n", score);

// 写入到标准错误
fprintf(stderr, "错误：参数无效\n");
```
### 常用格式说明符
- `%d`：输出十进制整数
- `%s`：输出字符串
- `%c`：输出单个字符
- `%f`：输出浮点数
- `%x`：输出十六进制整数
- `%p`：输出指针地址
- `%%`：输出百分号本身
  ```c
  fprintf(file, "数值: %10d 字符串: %-20s 浮点数: %.2f\n", 123, "test", 3.1415);
  ```

`======================================================================`

`======================================================================`
#### `fgetc`基本用法
```c
#include <stdio.h>
int fgetc(FILE *stream);
```
1. `stream`：文件流指针（`FILE*`类型，通过`fopen`获得或标准流如`stdin`）

#### 返回值
- 成功：返回读取的字符（以`int`类型表示，通常是`char`的ASCII值）
- 到达文件末尾：返回`EOF`
- 出错：返回`EOF`并设置`errno`（可通过`feof`和`ferror`区分两种情况）

```c
// 从文件逐个读取字符
FILE *file = fopen("text.txt", "r");
if (file != NULL) {
    int c;  // 必须用int类型接收，以区分EOF和普通字符
    // 循环读取直到文件结束
    while ((c = fgetc(file)) != EOF) {
        putchar(c);  // 输出到终端
    }
    // 检查结束原因
    if (feof(file)) {
        printf("\n文件读取完毕\n");
    } else if (ferror(file)) {
        perror("读取错误");
    }
    fclose(file);
}
// 从标准输入读取单个字符
printf("请输入一个字符: ");
int input = fgetc(stdin);
if (input != EOF) {
    printf("你输入了: %c\n", input);
}
```
### 关键特性
- 一次读取一个字符，文件内部指针会自动后移
- 读取文本文件时，会将换行符`'\n'`、制表符`'\t'`等控制字符正常返回
- 适合需要逐个处理字符的场景（如字符统计、简单解析等）

`======================================================================`

`======================================================================`
#### `fgets`基本用法
```c
#include <stdio.h>
char *fgets(char *str, int size, FILE *stream);
```
- 会自动在末尾补`\0`
1. `str`：指向存储读取数据的字符数组指针
2. `size`：最多读取的字符数（包含终止符`'\0'`）
3. `stream`：文件流指针（`FILE*`类型，通过`fopen`获得或标准流如`stdin`）

#### 返回值
- 成功：返回`str`（指向存储数据的缓冲区指针）
- 到达文件末尾或出错：返回`NULL`（可通过`feof`或`ferror`区分）

```c
// 从文件读取一行
FILE *file = fopen("notes.txt", "r");
if (file != NULL) {
    char buffer[100];  // 缓冲区，最多存储99个字符加终止符
    
    // 读取一行内容（最多99个字符）
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("读取到: %s", buffer);  // fgets会保留换行符
    }
    if (feof(file)) {
        printf("已读完文件\n");
    } else if (ferror(file)) {
        perror("读取错误");
    }
    fclose(file);
}
// 从标准输入读取用户输入
printf("请输入内容: ");
char input[50];
if (fgets(input, sizeof(input), stdin) != NULL) {
    printf("你输入了: %s", input);
}
```

`======================================================================`

`======================================================================`
#### `fscanf`基本用法
```c
#include <stdio.h>
int fscanf(FILE *stream, const char *format, ...);
```
1. `stream`：文件流指针（`FILE*`类型，如通过`fopen`打开的文件或`stdin`）
2. `format`：格式控制字符串，包含格式说明符（如`%d`、`%s`等）和可选的空白字符
3. `...`：可变参数列表，指向存储读取结果的变量地址

#### 返回值
- 成功：返回成功匹配并赋值的输入项数量
- 到达文件末尾：返回`EOF`
- 出错：返回`EOF`

```c
// 从文件读取格式化数据
FILE *data_file = fopen("records.txt", "r");
if (data_file != NULL) {
    int id;
    char name[50];
    float score;
    // 读取包含整数、字符串和浮点数的记录
    int result = fscanf(data_file, "%d %s %f", &id, name, &score);
    if (result == 3) {
        printf("读取到记录: ID=%d, 姓名=%s, 分数=%.1f\n", id, name, score);
    } else if (result == EOF) {
        if (feof(data_file)) {
            printf("已到达文件末尾\n");
        } else {
            perror("读取错误");
        }
    } else {
        printf("只成功读取了 %d 个字段\n", result);
    }
    fclose(data_file);
}
// 从标准输入读取用户输入
printf("请输入整数和字符串: ");
int num;
char str[30];
if (fscanf(stdin, "%d %s", &num, str) == 2) {
    printf("你输入了: %d 和 %s\n", num, str);
}
```
### 常用格式说明符
- `%d`：读取十进制整数
- `%s`：读取字符串（以空白字符为分隔符）
- `%c`：读取单个字符（包括空白字符）
- `%f`：读取浮点数
- `%x`：读取十六进制整数
- `%*d`：跳过一个整数（不存储）

### 关键特性
- 会自动跳过输入中的空白字符（空格、制表符、换行符等），除非使用`%c`或指定宽度
- `%s`读取字符串时，遇到空白字符停止，并在末尾添加`'\0'`
- 适合读取结构化的文本数据（如配置文件、日志记录等）


`======================================================================`

`======================================================================`
#### `exit`基本用法
```c
#include <stdlib.h>
void exit(int status);
```
1. `status`：程序的退出状态码，传递给父进程
   - 通常`0`表示程序正常退出
   - 非零值表示异常退出（具体含义可自定义）

#### 功能说明
- 终止当前进程的执行
- 执行一系列清理操作：
  - 调用所有通过`atexit`注册的函数（按注册相反顺序）
  - 刷新所有打开的文件流并关闭它们
  - 释放进程使用的资源（内存、文件描述符等）
- 将`status`返回给父进程（父进程可通过`wait`等函数获取）

```c
#include <stdlib.h>
#include <stdio.h>

void cleanup() {
    printf("执行清理操作...\n");
}

int main() {
    // 注册退出时要执行的函数
    atexit(cleanup);
    
    FILE *file = fopen("test.txt", "r");
    if (file == NULL) {
        perror("文件打开失败");
        exit(1);  // 异常退出，状态码1
    }
    
    // 程序正常执行完成
    fclose(file);
    exit(0);  // 正常退出，状态码0
}
```

`======================================================================`

`======================================================================`
## 标准输入、输出、错误
在Linux中，**标准输入（stdin）、标准输出（stdout）、标准错误（stderr）本质上被抽象成“文件”**，
程序通过操作它们的文件描述符（0、1、2）或标准库文件指针（`stdin`/`stdout`/`stderr`）来完成输入输出，
且它们在程序启动时就已被内核自动打开，无需手动调用`open()`或`fopen()`。
在Linux中，标准输入输出（Standard I/O）是程序与外部环境交互的基础，以下是相关的常用函数：

### 一、系统函数（直接操作文件描述符）
标准输入输出对应固定的文件描述符：
- `0`：标准输入（stdin，通常对应键盘）
- `1`：标准输出（stdout，通常对应终端）
- `2`：标准错误输出（stderr，通常对应终端）

常用系统函数：
1. `read(0, buf, count)` - 从标准输入读取数据
2. `write(1, buf, count)` - 向标准输出写入数据
3. `write(2, buf, count)` - 向标准错误输出写入数据
4. `dup2(oldfd, newfd)` - 重定向标准输入输出（如将stdout重定向到文件）

### 二、标准库函数（操作FILE*`指针`）
- `stdin`：标准输入（对应文件描述符0）
- `stdout`：标准输出（对应文件描述符1）
- `stderr`：标准错误输出（对应文件描述符2）

| 类型       | 含义               | 对应文件描述符 | C标准库文件指针 | 默认设备   | 核心特点                     |
|------------|--------------------|----------------|------------------|------------|------------------------------|
| **stdin**  | 标准输入           | 0              | `stdin`          | 键盘       | 程序读取外部输入的唯一标准接口 |
| **stdout** | 标准输出           | 1              | `stdout`         | 终端屏幕   | 行缓冲（换行时刷新），用于正常输出 |
| **stderr** | 标准错误输出       | 2              | `stderr`         | 终端屏幕   | 无缓冲（立即输出），用于错误信息 |

1. **输入函数**：
   - `scanf(const char *format, ...)` - 从stdin格式化读取
   - `fgets(char *s, int size, stdin)` - 从stdin读取一行
   - `getchar()` - 从stdin读取单个字符

2. **输出函数**：
   - `printf(const char *format, ...)` - 格式化输出到stdout
   - `puts(const char *s)` - 向stdout输出字符串（自动添加换行）
   - `putchar(int c)` - 向stdout输出单个字符
   - `fprintf(stderr, const char *format, ...)` - 向标准错误输出格式化内容

### 关键特性总结
1. **自动打开**：程序启动时，内核会自动创建这三个流，无需手动调用`open()`/`fopen()`，直接可用。
2. **缓冲差异**：
   - `stdout`是行缓冲，比如`printf("hello")`不会立即显示，加`\n`（`printf("hello\n")`）或调用`fflush(stdout)`才会刷新；
   - `stderr`无缓冲，比如`fprintf(stderr, "error")`会立即显示，确保错误信息不延迟。
3. **可重定向**：通过Shell命令（如`>`重定向`stdout`，`2>`重定向`stderr`）可将其指向文件/其他设备，不改变代码逻辑。  
   例：`./a.out > log.txt 2> err.txt`（正常输出写`log.txt`，错误写`err.txt`）。

例如，`printf("Hello\n")` 会最终调用 `write(1, "Hello\n", 6)` 完成输出。

`======================================================================`

`======================================================================`
