# log 日志库使用说明

## 概述
一个轻量级的C语言日志库，支持多种日志级别、自定义输出目标和线程安全，可灵活集成到各类C项目中。

## 基本用法

### 1. 包含头文件
```c
#include "log.h"
```

### 2. 日志级别
支持6种日志级别（从低到高）：
- `LOG_TRACE`：最详细的调试信息
- `LOG_DEBUG`：调试信息
- `LOG_INFO`：一般信息
- `LOG_WARN`：警告信息
- `LOG_ERROR`：错误信息
- `LOG_FATAL`：致命错误信息

### 3. 基本日志输出（最基本、最常用的功能）
直接使用对应级别的宏函数输出日志：
```c
LOG_TRACE("这是一条跟踪日志");
LOG_DEBUG("变量值: %d", value);
LOG_INFO("程序启动成功");
LOG_WARN("内存使用过高");
LOG_ERROR("文件打开失败: %s", filename);
LOG_FATAL("无法连接数据库，程序退出");
```

## 高级配置

### 1. 设置日志级别
过滤低于指定级别的日志（默认级别为`LOG_TRACE`）：
```c
// 只输出INFO及以上级别的日志
log_set_level(LOG_INFO);
```

### 2. 添加文件输出
除标准输出外，可将日志同时输出到文件：
```c
// 打开日志文件
FILE *logfile = fopen("app.log", "a");
if (logfile) {
  // 添加文件输出回调，设置级别为LOG_DEBUG（输出DEBUG及以上级别）
  log_add_fp(logfile, LOG_DEBUG);
}
```

### 3. 静默模式
禁用日志库默认的标准输出，仅保留通过`log_add_callback`（如文件输出、自定义回调）配置的日志输出渠道。
```c
// 开启静默模式（仅通过添加的回调输出日志）
log_set_quiet(true);
```

### 4. 线程安全设置
在多线程环境中，设置锁函数确保日志安全：
```c
// 示例：使用pthread mutex作为锁
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void lock_fn(bool lock, void *udata) {
  if (lock) {
    pthread_mutex_lock((pthread_mutex_t*)udata);
  } else {
    pthread_mutex_unlock((pthread_mutex_t*)udata);
  }
}

// 初始化锁
log_set_lock(lock_fn, &log_mutex);
```

### 5. 自定义日志回调
通过自定义回调实现特殊日志处理（如输出到网络）
1. **参数1：`custom_callback`**  
   用户自定义的日志处理函数（函数指针）
2. **参数2：`user_data`**  
   传给回调函数的自定义数据（无需求时可传 `NULL`）
3. **参数3：`LOG_ERROR`**  
   回调函数关注的日志级别
```c
// 定义自定义回调函数
static void custom_callback(log_Event *ev) {
  // 自定义处理逻辑，例如：
  // 1. 格式化日志内容
  // 2. 发送到网络服务
  // 3. 写入数据库等
}

// 添加自定义回调，设置关注的日志级别
log_add_callback(custom_callback, user_data, LOG_ERROR);
```

## 编译选项
- 启用彩色日志输出：编译时定义`LOG_USE_COLOR`宏
  ```bash
  gcc -DLOG_USE_COLOR ...
  ```

## 日志格式说明
- 标准输出（带颜色）：`时间 彩色级别 文件名:行号: 日志内容`
- 标准输出（无颜色）：`时间 级别 文件名:行号: 日志内容`
- 文件输出：`日期 时间 级别 文件名:行号: 日志内容`

## 注意事项
1. 最多支持32个自定义回调函数
2. 日志文件需要自行管理（如轮转、权限等）
3. 线程安全需要显式配置锁函数
4. 确保在程序退出前正确关闭日志文件（如果使用文件输出）

示例输出：
```
15:30:45 INFO  main.c:42: 程序启动
15:30:45 WARN  config.c:18: 未找到配置文件，使用默认配置
15:30:46 ERROR database.c:64: 连接超时
```