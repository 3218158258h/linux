# cJSON
cJSON 是一款**轻量级、无外部依赖、跨平台**的 C 语言 JSON 解析与构建库，专为资源受限场景（如嵌入式设备、物联网终端）设计。核心特点是代码极简（仅单个 `.c` 和 `.h` 文件）、API 直观、体积小巧，支持 JSON 数据的完整生命周期操作（解析/反序列化、构建/序列化、节点增删改查），是 C 语言开发中处理 JSON 数据的首选轻量库，广泛应用于物联网设备通信、嵌入式配置解析等场景。

常用实现：**官方原版 cJSON**（开源 MIT 协议，持续维护，地址：https://github.com/DaveGamble/cJSON）
## 一、核心头文件与依赖
- 核心头文件：`#include "cjson.h"`（本地源码集成时用双引号；系统安装后可改用 `<cjson/cjson.h>`）
- 编译依赖：无外部依赖库！仅需将 `cjson.c` 和 `cjson.h` 加入项目编译（例：`gcc -o app app.c cjson.c`）
- 安装/获取方式：
  1. 源码集成（推荐嵌入式场景）：从 GitHub 下载最新源码，直接复制两个核心文件到项目目录，无需额外配置
  2. 系统安装（Linux 桌面/服务器）：`sudo apt-get install libcjson-dev`（Ubuntu/Debian），编译时链接库 `-lcjson`
  3. 交叉编译：修改 Makefile 适配目标平台（ARM/MCU 等），无需额外依赖，适配性极强

## 二、核心结构体解析
### cJSON 结构体（JSON 节点核心结构）
cJSON 中所有 JSON 数据（对象、数组、字符串、数字等）均以 `cJSON` 结构体表示，每个节点包含类型、值、关联节点（子节点/兄弟节点）等信息，是所有操作的基础。结构体核心字段如下（简化版，完整字段见源码）：

| 字段名       | 类型         | 说明                                                                 |
|--------------|--------------|----------------------------------------------------------------------|
| `type`       | `cJSON_Type` | 节点数据类型（枚举值），核心类型：<br>- `cJSON_Number`：数字（整数/浮点数）<br>- `cJSON_String`：字符串<br>- `cJSON_Object`：JSON 对象（键值对集合）<br>- `cJSON_Array`：JSON 数组（有序元素集合）<br>- `cJSON_Bool`：布尔值（`cJSON_True`/`cJSON_False`）<br>- `cJSON_Null`：Null 值 |
| `valuedouble`| `double`     | 存储数字类型的值（`type=cJSON_Number` 时有效，兼容整数和浮点数）      |
| `valueint`   | `int`        | 存储整数类型的值（`type=cJSON_Number` 时有效，本质是 `valuedouble` 强制转换） |
| `valuestring`| `char*`      | 存储字符串类型的值（`type=cJSON_String` 时有效，动态分配内存，需通过库函数释放） |
| `string`     | `char*`      | 存储对象节点的键名（仅 `type=cJSON_Object` 的子节点有效，动态分配内存） |
| `child`      | `cJSON*`     | 指向子节点（`type=cJSON_Object` 或 `cJSON_Array` 时有效，子节点链表头） |
| `next`/`prev`| `cJSON*`     | 指向兄弟节点（同一对象/数组中的相邻节点，链表结构遍历用）             |

> ⚠️ 关键提醒：`cJSON` 结构体的内存由库函数动态分配（`malloc`），**必须通过 `cJSON_Delete` 递归释放**，否则会导致内存泄漏！

## 三、核心函数详细解析
### 1. 解析 JSON 字符串（反序列化）：`cJSON_Parse`
#### 函数原型
```c
cJSON* cJSON_Parse(const char* value);
```
#### 作用
将 JSON 格式字符串解析为 `cJSON` 结构体对象树（根节点），后续可通过节点操作函数提取数据。

#### 参数解析
| 参数名  | 类型         | 说明                                                                 |
|---------|--------------|----------------------------------------------------------------------|
| `value` | `const char*` | 输入 JSON 字符串（必须以 `'\0'` 结尾，格式需合法），例：`{"name":"Alice","age":25}` |

#### 返回值
- 成功：返回 `cJSON*` 根节点指针（需通过 `cJSON_Delete` 释放）
- 失败：返回 `NULL`（原因：JSON 格式非法、字符串未终止、内存分配失败）

#### 扩展函数（推荐）：`cJSON_ParseWithLength`
```c
cJSON* cJSON_ParseWithLength(const char* value, size_t length);
```
- 优势：指定字符串长度，无需依赖 `'\0'` 终止符，避免非法字符串导致的越界问题
- 场景：处理网络传输、文件读取的 JSON 数据（可能不含终止符）

#### 错误调试：`cJSON_GetErrorPtr`
```c
const char* cJSON_GetErrorPtr(void);
```
- 作用：`cJSON_Parse` 失败时，返回解析错误的字符串位置，便于定位格式问题（例：引号未闭合、逗号多余）

### 2. 构建 JSON 根节点：`cJSON_CreateObject` / `cJSON_CreateArray`
#### （1）创建 JSON 对象（键值对集合）
```c
cJSON* cJSON_CreateObject(void);
```
- 作用：创建空的 JSON 对象根节点（`type=cJSON_Object`），后续可添加键值对
- 返回值：成功返回节点指针（需释放），失败返回 `NULL`（内存不足）

#### （2）创建 JSON 数组（有序元素集合）
```c
cJSON* cJSON_CreateArray(void);
```
- 作用：创建空的 JSON 数组根节点（`type=cJSON_Array`），后续可添加元素
- 返回值：成功返回节点指针（需释放），失败返回 `NULL`

### 3. 向 JSON 对象添加键值对（便捷函数）
cJSON 提供直接添加指定类型值的便捷函数（无需手动创建子节点），核心函数如下：

| 函数原型                                                                 | 作用                                  | 核心参数说明                                                                 |
|--------------------------------------------------------------------------|---------------------------------------|------------------------------------------------------------------------------|
| `cJSON_bool cJSON_AddStringToObject(cJSON* obj, const char* key, const char* val)` | 添加字符串类型键值对                  | `val` 可为 `NULL`（对应 JSON 的 `null`）                                      |
| `cJSON_bool cJSON_AddNumberToObject(cJSON* obj, const char* key, double val)`     | 添加数字类型键值对（兼容整数/浮点数） | 整数直接传入（如 `25` 自动转为 `double`）                                     |
| `cJSON_bool cJSON_AddBoolToObject(cJSON* obj, const char* key, cJSON_bool val)`   | 添加布尔类型键值对                    | `val` 取值：`cJSON_True` / `cJSON_False`                                      |
| `cJSON_bool cJSON_AddNullToObject(cJSON* obj, const char* key)`                    | 添加 Null 类型键值对                  | 用于表示字段未定义、数据缺失                                                |
| `cJSON_bool cJSON_AddItemToObject(cJSON* obj, const char* key, cJSON* item)`     | 添加子节点（对象/数组）               | `item` 为已创建的子节点（无需手动释放，根节点释放时递归释放）                 |

#### 示例（添加嵌套对象）
```c
cJSON* root = cJSON_CreateObject();
cJSON* address = cJSON_CreateObject(); // 创建子对象
cJSON_AddStringToObject(address, "city", "Beijing");
cJSON_AddItemToObject(root, "address", address); // 嵌套到根对象
```

### 4. 向 JSON 数组添加元素（便捷函数）
| 函数原型                                                                 | 作用                                  |
|--------------------------------------------------------------------------|---------------------------------------|
| `cJSON_bool cJSON_AddStringToArray(cJSON* arr, const char* val)`         | 添加字符串元素                        |
| `cJSON_bool cJSON_AddNumberToArray(cJSON* arr, double val)`              | 添加数字元素                          |
| `cJSON_bool cJSON_AddBoolToArray(cJSON* arr, cJSON_bool val)`            | 添加布尔元素                          |
| `cJSON_bool cJSON_AddNullToArray(cJSON* arr)`                            | 添加 Null 元素                        |
| `cJSON_bool cJSON_AddItemToArray(cJSON* arr, cJSON* item)`               | 添加子节点（对象/数组）               |

#### 示例（创建数组并添加元素）
```c
cJSON* scores = cJSON_CreateArray();
cJSON_AddNumberToArray(scores, 90.5);
cJSON_AddNumberToArray(scores, 85);
cJSON_AddItemToObject(root, "scores", scores); // 关联到根对象
```

### 5. 获取 JSON 节点（解析后取值）
#### （1）获取对象节点（按键名）
```c
cJSON* cJSON_GetObjectItem(const cJSON* obj, const char* key);
```
- 作用：从 JSON 对象中根据键名获取子节点
- 返回值：找到返回节点指针，未找到返回 `NULL`

#### （2）获取数组元素（按索引）
```c
cJSON* cJSON_GetArrayItem(const cJSON* arr, int index);
```
- 作用：从 JSON 数组中按索引（0 开始）获取元素
- 返回值：索引合法返回节点指针，否则返回 `NULL`

#### （3）获取数组长度
```c
int cJSON_GetArraySize(const cJSON* arr);
```
- 作用：获取 JSON 数组的元素个数（仅 `type=cJSON_Array` 有效）

### 6. 节点修改（更新已有节点值）
#### 核心函数（规范修改方式）
| 函数原型                                                                 | 作用                                  | 参数解析                                                                 |
|--------------------------------------------------------------------------|---------------------------------------|--------------------------------------------------------------------------|
| `void cJSON_SetNumberValue(cJSON* item, double value)`                   | 修改数字类型节点值                    | `item`：已获取的数字节点；`value`：新值（整数/浮点数均可）                |
| `void cJSON_SetStringValue(cJSON* item, const char* string)`             | 修改字符串类型节点值                  | `item`：已获取的字符串节点；`string`：新字符串（库会自动复制，无需手动分配） |
| `void cJSON_SetBoolValue(cJSON* item, cJSON_bool value)`                 | 修改布尔类型节点值                    | `item`：已获取的布尔节点；`value`：`cJSON_True`/`cJSON_False`             |
| `void cJSON_SetNullValue(cJSON* item)`                                   | 将节点值设为 Null                     | `item`：任意类型节点（修改后类型变为 `cJSON_Null`）                       |

#### 作用
更新已存在节点的值（支持数字、字符串、布尔、Null 类型），相比直接修改结构体字段更规范，避免内存操作失误。

#### 关键注意事项
- 修改前必须用 `cJSON_IsXXX` 验证节点类型（例：仅对 `cJSON_IsString` 为真的节点调用 `cJSON_SetStringValue`）；
- `cJSON_SetStringValue` 会自动释放节点原有 `valuestring` 内存，无需手动 `free`，避免泄漏。

### 7. 节点删除（移除对象/数组中的节点）
#### 核心函数
##### （1）删除对象中的字段
```c
cJSON_bool cJSON_DeleteItemFromObject(cJSON* obj, const char* key);
```
- 作用：根据键名删除 JSON 对象中的指定字段（含子节点递归释放）
- 参数解析：
  | 参数名  | 类型         | 说明                                  |
  |---------|--------------|---------------------------------------|
  | `obj`   | `cJSON*`     | 目标 JSON 对象节点                    |
  | `key`   | `const char*`| 要删除的字段键名                      |
- 返回值：`cJSON_True`（删除成功）/ `cJSON_False`（字段不存在/节点无效）

##### （2）删除数组中的元素
```c
cJSON_bool cJSON_DeleteItemFromArray(cJSON* arr, int index);
```
- 作用：根据索引删除 JSON 数组中的指定元素（含子节点递归释放）
- 参数解析：
  | 参数名  | 类型         | 说明                                  |
  |---------|--------------|---------------------------------------|
  | `arr`   | `cJSON*`     | 目标 JSON 数组节点                    |
  | `index` | `int`        | 要删除的元素索引（0 开始，负数无效）  |
- 返回值：`cJSON_True`（删除成功）/ `cJSON_False`（索引越界/节点无效）

#### 关键注意事项
- 删除后节点内存会被递归释放，不可再访问该节点指针；
- 支持嵌套对象/数组的删除（例：删除对象中嵌套数组的元素，需先获取数组节点再调用函数）。

### 8. 节点替换（用新节点替换旧节点）
#### 核心函数
##### （1）替换对象中的字段
```c
cJSON_bool cJSON_ReplaceItemInObject(cJSON* obj, const char* key, cJSON* newitem);
```
- 作用：用新节点替换 JSON 对象中指定键名的旧节点（自动释放旧节点内存）
- 参数解析：
  | 参数名    | 类型         | 说明                                  |
  |-----------|--------------|---------------------------------------|
  | `obj`     | `cJSON*`     | 目标 JSON 对象节点                    |
  | `key`     | `const char*`| 要替换的字段键名                      |
  | `newitem` | `cJSON*`     | 新节点（需通过 `cJSON_CreateXXX` 创建）|
- 返回值：`cJSON_True`（替换成功）/ `cJSON_False`（字段不存在/新节点无效）

##### （2）替换数组中的元素
```c
cJSON_bool cJSON_ReplaceItemInArray(cJSON* arr, int index, cJSON* newitem);
```
- 作用：用新节点替换 JSON 数组中指定索引的旧节点（自动释放旧节点内存）
- 参数解析：
  | 参数名    | 类型         | 说明                                  |
  |-----------|--------------|---------------------------------------|
  | `arr`     | `cJSON*`     | 目标 JSON 数组节点                    |
  | `index`   | `int`        | 要替换的元素索引（0 开始）            |
  | `newitem` | `cJSON*`     | 新节点（需通过 `cJSON_CreateXXX` 创建）|
- 返回值：`cJSON_True`（替换成功）/ `cJSON_False`（索引越界/新节点无效）

#### 关键注意事项
- 新节点必须是通过 `cJSON_CreateXXX` 函数创建的有效节点，不可传入栈内存节点；
- 旧节点会被自动递归释放，无需手动调用 `cJSON_Delete(olditem)`；
- 支持跨类型替换（例：用数字节点替换字符串节点、用对象节点替换数组节点）。

### 9. 序列化 JSON 对象（转为字符串）：`cJSON_Print` / `cJSON_PrintUnformatted`
#### 函数原型
```c
char* cJSON_Print(const cJSON* item);       // 格式化输出（带缩进）
char* cJSON_PrintUnformatted(const cJSON* item); // 紧凑输出（无缩进）
```
#### 作用
将 `cJSON` 对象树转为 JSON 字符串，用于网络传输、文件存储等场景。

#### 返回值
- 成功：返回动态分配的 JSON 字符串（需手动 `free` 释放）
- 失败：返回 `NULL`（内存不足、节点无效）

#### 区别与适用场景
| 函数                  | 输出效果                                  | 适用场景                          |
|-----------------------|-------------------------------------------|-----------------------------------|
| `cJSON_Print`         | 带缩进、换行，可读性强（例：`{"name":"Alice","age":25}` 格式化后带换行） | 调试、日志输出                    |
| `cJSON_PrintUnformatted` | 无缩进、无换行，体积小（例：`{"name":"Alice","age":25}`） | 网络传输、文件存储（节省带宽/空间） |

### 10. 类型判断宏（关键！避免类型错误）
获取节点后必须通过以下宏判断类型，否则访问 `valuestring`/`valueint` 会导致崩溃或脏数据：

| 宏定义                | 作用                                  |
|-----------------------|---------------------------------------|
| `cJSON_IsString(node)`| 判断节点是否为字符串类型              |
| `cJSON_IsNumber(node)`| 判断节点是否为数字类型                |
| `cJSON_IsObject(node)`| 判断节点是否为对象类型                |
| `cJSON_IsArray(node)` | 判断节点是否为数组类型                |
| `cJSON_IsBool(node)`  | 判断节点是否为布尔类型                |
| `cJSON_IsNull(node)`  | 判断节点是否为 Null 类型              |

### 11. 释放内存：`cJSON_Delete`（重中之重）
#### 函数原型
```c
void cJSON_Delete(cJSON* item);
```
#### 作用
递归释放 `cJSON` 节点及其所有子节点、兄弟节点的内存（包括 `valuestring`、`string` 等动态字段），是避免内存泄漏的核心函数。

#### 关键注意事项
1. 仅需释放根节点：子节点会被递归释放，无需单独释放（重复释放会崩溃）
2. 释放后指针失效：不可再访问该指针指向的内存（野指针）
3. 空指针安全：传入 `NULL` 无操作（可放心调用 `cJSON_Delete(NULL)`）
4. 嵌入式场景：需及时释放，避免堆内存溢出

## 四、完整示例（生产环境常用）
### 示例 1：构建 JSON 并序列化（发送数据场景）
```c
#include <stdio.h>
#include <stdlib.h>
#include "cjson.h"

int main(void) {
    // 1. 创建根对象
    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        printf("创建 JSON 失败（内存不足）\n");
        return -1;
    }

    // 2. 添加基础键值对
    cJSON_AddStringToObject(root, "device_id", "sensor_001");
    cJSON_AddNumberToObject(root, "temperature", 23.5);
    cJSON_AddNumberToObject(root, "humidity", 65);
    cJSON_AddBoolToObject(root, "is_online", cJSON_True);
    cJSON_AddNullToObject(root, "gps_data"); // 添加 Null 字段

    // 3. 添加嵌套对象
    cJSON* location = cJSON_CreateObject();
    cJSON_AddStringToObject(location, "city", "Shanghai");
    cJSON_AddStringToObject(location, "lat", "31.2304");
    cJSON_AddStringToObject(location, "lon", "121.4737");
    cJSON_AddItemToObject(root, "location", location);

    // 4. 添加数组
    cJSON* data_points = cJSON_CreateArray();
    cJSON_AddNumberToArray(data_points, 23.2);
    cJSON_AddNumberToArray(data_points, 23.4);
    cJSON_AddNumberToArray(data_points, 23.5);
    cJSON_AddItemToObject(root, "data_points", data_points);

    // 5. 序列化（紧凑格式，用于网络传输）
    char* json_compact = cJSON_PrintUnformatted(root);
    if (json_compact) {
        printf("紧凑 JSON：%s\n", json_compact);
        free(json_compact); // 释放序列化字符串
    }

    // 6. 序列化（格式化格式，用于调试）
    char* json_formatted = cJSON_Print(root);
    if (json_formatted) {
        printf("\n格式化 JSON：\n%s\n", json_formatted);
        free(json_formatted);
    }

    // 7. 释放 JSON 对象（递归释放所有节点）
    cJSON_Delete(root);
    return 0;
}
```

### 示例 2：解析 JSON 字符串（接收数据场景）
```c
#include <stdio.h>
#include <stdlib.h>
#include "cjson.h"

int main(void) {
    // 模拟接收的 JSON 数据（如 MQTT 消息、网络响应）
    const char* received_json = "{\"device_id\":\"sensor_001\",\"temperature\":23.5,"
                               "\"humidity\":65,\"is_online\":true,"
                               "\"location\":{\"city\":\"Shanghai\",\"lat\":\"31.2304\",\"lon\":\"121.4737\"},"
                               "\"data_points\":[23.2,23.4,23.5],\"gps_data\":null}";

    // 1. 解析 JSON 字符串
    cJSON* root = cJSON_Parse(received_json);
    if (root == NULL) {
        printf("JSON 解析失败：错误位置=%s\n", cJSON_GetErrorPtr());
        return -1;
    }

    // 2. 提取字符串类型
    cJSON* dev_id_node = cJSON_GetObjectItem(root, "device_id");
    if (dev_id_node && cJSON_IsString(dev_id_node)) {
        printf("设备 ID：%s\n", dev_id_node->valuestring);
    }

    // 3. 提取数字类型
    cJSON* temp_node = cJSON_GetObjectItem(root, "temperature");
    if (temp_node && cJSON_IsNumber(temp_node)) {
        printf("温度：%.1f℃\n", temp_node->valuedouble);
    }

    // 4. 提取布尔类型
    cJSON* online_node = cJSON_GetObjectItem(root, "is_online");
    if (online_node && cJSON_IsBool(online_node)) {
        printf("在线状态：%s\n", online_node->valueint ? "在线" : "离线");
    }

    // 5. 提取 Null 类型
    cJSON* gps_node = cJSON_GetObjectItem(root, "gps_data");
    if (gps_node && cJSON_IsNull(gps_node)) {
        printf("GPS 状态：数据缺失\n");
    }

    // 6. 提取嵌套对象
    cJSON* loc_node = cJSON_GetObjectItem(root, "location");
    if (loc_node && cJSON_IsObject(loc_node)) {
        cJSON* city_node = cJSON_GetObjectItem(loc_node, "city");
        if (city_node && cJSON_IsString(city_node)) {
            printf("城市：%s\n", city_node->valuestring);
        }
    }

    // 7. 提取数组并遍历
    cJSON* data_node = cJSON_GetObjectItem(root, "data_points");
    if (data_node && cJSON_IsArray(data_node)) {
        int size = cJSON_GetArraySize(data_node);
        printf("数据点（%d 个）：", size);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(data_node, i);
            if (cJSON_IsNumber(item)) {
                printf("%.1f ", item->valuedouble);
            }
        }
        printf("\n");
    }

    // 8. 释放内存
    cJSON_Delete(root);
    return 0;
}
```

### 示例 3：节点修改、删除、替换综合操作
```c
#include <stdio.h>
#include <stdlib.h>
#include "cjson.h"

int main(void) {
    // 原始 JSON 数据
    const char* origin_json = "{\"device_id\":\"sensor_001\",\"temperature\":23.5,"
                               "\"is_online\":true,\"version\":\"v1.0\","
                               "\"logs\":[\"boot\",\"connect\",\"data_send\"],"
                               "\"config\":{\"mode\":\"auto\"}}";
    
    // 1. 解析 JSON
    cJSON* root = cJSON_Parse(origin_json);
    if (root == NULL) {
        printf("解析失败：%s\n", cJSON_GetErrorPtr());
        return -1;
    }
    printf("=== 原始 JSON ===\n%s\n", cJSON_Print(root));

    // 2. 节点修改：更新温度和设备ID
    cJSON* temp_node = cJSON_GetObjectItem(root, "temperature");
    if (temp_node && cJSON_IsNumber(temp_node)) {
        cJSON_SetNumberValue(temp_node, 24.1); // 规范修改数字值
    }
    cJSON* dev_id_node = cJSON_GetObjectItem(root, "device_id");
    if (dev_id_node && cJSON_IsString(dev_id_node)) {
        cJSON_SetStringValue(dev_id_node, "sensor_001_updated"); // 规范修改字符串值
    }

    // 3. 节点删除：删除无用的 "version" 字段和数组第2个元素
    if (cJSON_DeleteItemFromObject(root, "version") == cJSON_True) {
        printf("\n=== 已删除字段：version ===\n");
    }
    cJSON* logs_array = cJSON_GetObjectItem(root, "logs");
    if (logs_array && cJSON_IsArray(logs_array)) {
        if (cJSON_DeleteItemFromArray(logs_array, 1) == cJSON_True) {
            printf("=== 已删除数组索引1的元素 ===\n");
        }
    }

    // 4. 节点替换：将简单 config 替换为复杂对象
    cJSON* old_config = cJSON_GetObjectItem(root, "config");
    cJSON* new_config = cJSON_CreateObject();
    cJSON_AddStringToObject(new_config, "mode", "manual");
    cJSON_AddNumberToObject(new_config, "interval", 5);
    cJSON_AddBoolToObject(new_config, "enable_log", cJSON_True);
    if (old_config && new_config) {
        cJSON_ReplaceItemInObject(root, "config", new_config);
        printf("=== 已替换 config 子对象 ===\n");
    }

    // 5. 输出最终结果
    printf("\n=== 最终 JSON ===\n%s\n", cJSON_Print(root));

    // 6. 释放资源
    cJSON_Delete(root);
    return 0;
}
```

## 五、生产环境关键注意事项
### 1. 内存管理（核心优先级最高）
- 牢记“谁创建谁释放”：`cJSON_Parse`/`cJSON_CreateXXX` 创建的节点 → 用 `cJSON_Delete` 释放；`cJSON_Print` 生成的字符串 → 用 `free` 释放
- 嵌入式设备：建议通过 `cJSON_Hooks` 自定义内存分配函数（替换默认 `malloc`/`free`），结合内存池减少碎片
- 节点修改/替换：`cJSON_SetStringValue`/`cJSON_ReplaceItemInObject` 会自动释放旧内存，无需手动干预

### 2. 类型安全不可忽视
- 任何节点取值、修改前必须用 `cJSON_IsXXX` 判断类型，严禁直接访问 `valuestring`/`valueint`（例：将字符串节点当作数字访问会崩溃）
- 跨类型替换需谨慎（例：用数组节点替换字符串节点后，后续操作需按数组类型处理）

### 3. 错误处理必须完善
- `cJSON_Parse` 失败时，用 `cJSON_GetErrorPtr` 定位格式问题，便于调试
- 所有 `cJSON_AddXXX`/`cJSON_DeleteXXX`/`cJSON_ReplaceXXX` 函数返回值需检查（返回 `cJSON_False` 时可能是内存不足或节点无效，需处理）

### 4. 嵌入式适配优化
- 裁剪功能：通过宏定义关闭无用功能（如 `CJSON_NANO` 关闭注释解析、格式化输出，减小代码体积）
- 堆内存配置：嵌入式设备堆默认较小，解析大 JSON 时需扩大堆（如 ARM Cortex-M 中配置 `__heap_size`）
- 批量操作：大批量添加数组元素时，避免频繁调用 `cJSON_AddXXXToArray`，可优化为一次性构建后关联

### 5. 性能优化建议
- 大 JSON 优先用 `cJSON_ParseWithLength` 和 `cJSON_PrintUnformatted`（减少内存占用和处理时间）
- 频繁创建/释放 JSON 的场景，使用内存池管理，避免堆碎片
- 遍历复杂 JSON 时，递归深度不宜过深（嵌入式设备栈空间有限，建议用迭代遍历替代递归）

## 六、核心函数速查表（全场景覆盖）
| 操作类型       | 核心函数                                  | 用途                                  |
|----------------|-------------------------------------------|---------------------------------------|
| 解析           | `cJSON_Parse`/`cJSON_ParseWithLength`      | JSON 字符串转对象树                   |
| 构建           | `cJSON_CreateObject`/`cJSON_CreateArray`   | 创建对象/数组根节点                   |
| 添加           | `cJSON_AddXXXToObject`/`cJSON_AddXXXToArray` | 向对象/数组添加键值对/元素            |
| 获取           | `cJSON_GetObjectItem`/`cJSON_GetArrayItem`  | 按键名/索引获取节点                   |
| 修改           | `cJSON_SetNumberValue`/`cJSON_SetStringValue` | 修改节点值（规范方式）                |
| 删除           | `cJSON_DeleteItemFromObject`/`cJSON_DeleteItemFromArray` | 删除对象字段/数组元素                 |
| 替换           | `cJSON_ReplaceItemInObject`/`cJSON_ReplaceItemInArray` | 替换对象/数组中的节点                 |
| 序列化         | `cJSON_Print`/`cJSON_PrintUnformatted`     | 对象树转 JSON 字符串                  |
| 类型判断       | `cJSON_IsString`/`cJSON_IsNumber`等        | 验证节点类型                          |
| 内存释放       | `cJSON_Delete`                             | 递归释放节点内存                      |
| Null值处理     | `cJSON_AddNullToObject`/`cJSON_IsNull`     | 添加/判断 Null 节点                   |
| 数组辅助       | `cJSON_GetArraySize`                       | 获取数组长度                          |
