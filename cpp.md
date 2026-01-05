
### 命名空间
在 C++ 中，**命名空间（Namespace）** 是用于解决命名冲突的核心机制，它允许将全局作用域划分为多个独立的、可命名的作用域，避免不同模块/库中的标识符（变量、函数、类等）重名。

C++ 标准库（比如 `cout`、`cin`、`endl` 这些工具）里的所有内容，都被放在一个名为 `std` 的“命名空间”中。  
命名空间的作用是**避免名字冲突**——比如，你自己写的代码里可能有一个叫 `cout` 的变量，这就会和标准库的 `cout` 重名，而命名空间可以区分它们。

- `using namespace std;` 的作用
这句话的意思是：**“我要使用 `std` 这个命名空间里的所有内容”**。  
加上这句话后，你可以直接写 `cout`、`cin`、`endl` 等，不用额外说明它们来自 `std` 命名空间。

- 如果不写 `using namespace std;`，就必须在标准库的工具前加上 `std::` 前缀，明确说明“这个工具来自 `std` 命名空间”。  

#### 核心作用
1. **避免命名冲突**：例如两个库都定义了 `print()` 函数，可通过命名空间区分；
2. **模块化管理代码**：将相关的代码（如工具函数、类）归类到同一个命名空间，增强代码可读性和维护性；
3. **控制作用域**：限制标识符的可见范围，避免全局作用域污染。


#### 基本语法
**定义命名空间**
使用 `namespace` 关键字定义，语法如下：
```cpp
// 普通命名空间
namespace 命名空间名 {
    // 变量、函数、类、结构体等
    int num = 10;
    void print() {
        std::cout << "Hello from namespace!" << std::endl;
    }

    // 命名空间内可嵌套其他命名空间
    namespace inner {
        double pi = 3.14159;
    }
}
```

**注意**：
- 命名空间的定义可以跨文件（即拆分到多个 `.cpp`/`.h` 文件中），编译器会自动合并同名命名空间；
- 命名空间不影响变量/函数的存储位置（全局命名空间的变量仍在全局区），仅影响作用域。

**访问命名空间中的成员**
有 3 种方式访问命名空间内的标识符：

- 作用域解析符 `::`（最常用）
```cpp
#include <iostream>
namespace MyNS {
    int num = 10;
    void print() { std::cout << num << std::endl; }
}

int main() {
    // 直接通过 命名空间名::标识符 访问
    std::cout << MyNS::num << std::endl; // 输出 10
    MyNS::print(); // 输出 10
    return 0;
}
```

- `using` 声明（引入单个成员）
```cpp
#include <iostream>
namespace MyNS {
    int num = 10;
    void print() { std::cout << num << std::endl; }
}

// 引入 MyNS 中的 num，后续可直接使用 num
using MyNS::num;
int main() {
    std::cout << num << std::endl; // 等价于 MyNS::num
    MyNS::print(); // print 未被引入，仍需加命名空间
    return 0;
}
```

- `using namespace`（引入整个命名空间）
```cpp
#include <iostream>
namespace MyNS {
    int num = 10;
    void print() { std::cout << num << std::endl; }
}

// 引入 MyNS 所有成员，后续可直接使用
using namespace MyNS;
int main() {
    std::cout << num << std::endl; // 直接使用 num
    print(); // 直接使用 print
    return 0;
}
```

**⚠️ 注意**：`using namespace std;` 是最常见的用法（引入标准库命名空间），但在头文件中应避免使用——会导致全局作用域污染，建议仅在 `.cpp` 文件中使用，或直接用 `std::` 限定（如 `std::cout`）。


#### 高级特性
1. 嵌套命名空间
命名空间可以嵌套，访问时需逐层指定：
```cpp
namespace Outer {
    namespace Inner {
        int value = 20;
    }
}
int main() {
    // 逐层访问
    std::cout << Outer::Inner::value << std::endl; // 输出 20
    // C++17 简化嵌套命名空间定义
    namespace Outer::Inner::Deep {
        int deep_val = 30;
    }
    std::cout << Outer::Inner::Deep::deep_val << std::endl; // 输出 30
    return 0;
}
```

2. 匿名命名空间
无名称的命名空间，作用域仅限于当前编译单元（`.cpp` 文件），等价于给标识符加 `static`（但更通用，支持类、函数等）：
```cpp
// 匿名命名空间：仅当前文件可见
namespace {
    int local_num = 100; // 其他文件无法访问
    void local_func() { std::cout << "Local function" << std::endl; }
}
int main() {
    local_func(); // 直接访问，无需命名空间
    return 0;
}
```

3. 命名空间别名
给长命名空间起短别名，简化代码：
```cpp
// 原命名空间名较长
namespace VeryLongNamespaceName {
    int data = 5;
}

// 定义别名
namespace VLN = VeryLongNamespaceName;

int main() {
    std::cout << VLN::data << std::endl; // 输出 5
    return 0;
}
```

4. 命名空间的合并
同名命名空间会被编译器自动合并（即使定义在不同位置/文件）：
```cpp
// 第一个定义
namespace MyNS {
    int a = 1;
}
// 第二个定义（自动合并）
namespace MyNS {
    int b = 2;
}
```


#### 标准库命名空间 `std`
C++ 标准库的所有标识符（如 `cout`、`string`、`vector` 等）都定义在 `std` 命名空间中：
```cpp
#include <iostream> // cout 位于 std 中
#include <string>   // string 位于 std 中

int main() {
    // 方式1：显式使用 std::
    std::string str = "Hello";
    std::cout << str << std::endl;
    // 方式2：局部引入（推荐，避免全局污染）
    using std::cout;
    using std::string;
    string str2 = "World";
    cout << str2 << std::endl;
    // 方式3：全局引入（简单但不推荐在头文件中用）
    // using namespace std;
    return 0;
}
```


#### 最佳实践
1. **头文件中禁用 `using namespace`**：避免污染包含该头文件的其他代码的全局作用域；
2. **优先使用 `std::` 或局部 `using` 声明**：如 `using std::cout;`，而非全局引入 `std`；
3. **模块化代码**：将相关功能归类到独立命名空间（如 `Math`、`Network`）；
4. **匿名命名空间替代 `static`**：在编译单元内隐藏标识符时，优先用匿名命名空间（支持更多类型）；
5. **避免命名空间嵌套过深**：嵌套层数过多会降低代码可读性。



### 标准输出
```c++
cout << 数据
```
- cout << 1 << 2 << 3;     结果: 123
- cout << endl;     结果: 换行
- cout << 1 << 2 << endl;    结果: 12后换行
- cout << "hello world";     结果: hello world


### 标准输入
```c++
cin >> 变量
```
- cin >> a;   输入1，a=1
- cin >> a >> b ;  输入 1 2，a=1 b=2


```cpp
#include <iostream>
using namespace std;

int main() {
    int a, b;
    double c;
    cout << "请输入两个整数和一个小数（用空格分隔）：";
    cin >> a >> b >> c;  // 连续读取，自动匹配变量类型
    cout << "结果：" << a << ", " << b << ", " << c << endl;
    return 0;
}
```

示例：  
`10 20 3.14`  
结果：  
`结果：10, 20, 3.14`  



### 关键字const
`const` 用于声明**常量**（即值不能被修改的变量），作用是“只读保护”，让代码更安全、更易维护。

- 定义常量
```c
// 定义一个整数常量，值不能被修改
const int MAX_NUM = 100;
// 错误示例：尝试修改 const 变量会编译报错
// MAX_NUM = 200;  // 编译错误：assignment of read-only variable
```
- `const` 修饰的变量**必须在定义时初始化**（否则后续无法赋值）。

1. **修饰指针指向的值（常量指针）**  
   指针指向的值不能改，但指针本身可以指向其他地址：
   ```c
   int a = 10, b = 20;
   const int* p = &a;  // 等价于 int const* p = &a;
   // *p = 30;  // 错误：不能修改指向的值
   p = &b;  // 正确：指针可以指向其他地址
   ```

2. **修饰指针本身（指针常量）**  
   指针本身不能改（不能指向其他地址），但指向的值可以改：
   ```c
   int a = 10, b = 20;
   int* const p = &a;  // 指针本身是常量
   *p = 30;  // 正确：可以修改指向的值（a 变成 30）
   // p = &b;  // 错误：指针不能指向其他地址
   ```

3. **既修饰指针又修饰指向的值**  
   指针和指向的值都不能改：
   ```c
   const int* const p = &a;
   // *p = 30;  // 错误
   // p = &b;   // 错误
   ```
4. **修饰成员函数（常函数）**  
在成员函数的参数列表后、函数体前加 const
- 本质是修改this指针
- 普通成员函数this指针本身不可改，指向的对象可改
- 常成员函数this指针不可改，`指向的对象不可改`
**表现**
- 不能调用非const成员函数
- 不能修改非静态成员变量（可读不可写）
  - 用mutable修饰后可写，如mutable int a;
cpp
```cpp
class 类名 {
public:
    返回值类型 函数名(参数列表) const { 函数体 }
};
```
5. **修饰对象（常对象）**
- 常对象只能调用常成员函数，不能调用普通成员函数，因为普通成员函数会可以修改属性
```cpp
const Person p;//对象p的属性不可修改，但是mutable修饰的属性可修改（同上）
```

### 关键字static
- 定义在函数内部则作用域在内部，每次调用不会清零
- 作用域在cpp文件则作用域限定在本文件
- 定义在类内则作用域为类，必须在类外初始化，每个对象对于同一个staic共享数据，不同static数据不共享
- 所有对象的同一静态成员变量共享数据，不同静态成员变量不共享数据
- 类中定义的静态成员变量在编译阶段分配内存;必须类外初始化，私有数据不影响初始化，但是影响读写
- 所有对象的共享同一静态成员函数
- 类中的静态成员函数只能直接访问静态成员（包括静态成员变量和静态成员函数），无法直接访问非静态成员（非静态成员变量、非静态成员函数）
- 静态成员变量不存储在类空间中，不占用对象的空间，非静态成员变量存储在类空间中
```cpp
class Person
{
public:

    int test;
    //编译阶段分配内存
    static int age;
    //静态成员函数
    //只能访问静态成员变量
    static void hello()
    {
        cout << age <<endl;
        cout << "Hello World!" << endl;
    }
    Person():test(2){}

private:
    static int count;

};
//类外初始化静态成员变量
int Person::age = 10;
//类外初始化静态成员变量，私有数据也可以类外初始化
//但是不可以调用
int Person::count = 0;
int main()
{
    Person p,x;
    p.hello();
    p.age = 1;
    x.hello();//由于所有对象的静态成员变量是共享的，所以x.age=1
    return 0;
}
```

### string类型
在 C++ 中，`std::string` 是标准库提供的字符串类型，用于方便地处理文本数据。它封装了字符串的存储和操作逻辑，相比 C 语言中的字符数组（`char[]`）更加安全、易用。
1. **动态大小**：自动管理内存，无需手动分配/释放空间
2. **丰富的成员函数**：提供了大量字符串操作方法（拼接、查找、替换等）
3. **兼容性**：可与 C 风格字符串（`char*`）相互转换
4. **安全性**：避免了字符数组常见的越界问题

#### 基本用法
```cpp
#include <iostream>
#include <string>  // 必须包含此头文件
using namespace std;  // 否则需使用 std::string
int main() {
    // 1. 字符串初始化
   string s1;                  // 空字符串
   string s2 = "hello";        // 直接初始化,相当于先构造一个字符串再赋值给 s2
   string s3("world");         // 构造函数初始化
   string s4(5, 'a');          // 重复字符初始化："aaaaa"
   string s5 = s2 + " " + s3;  // 字符串拼接："hello world"
    // 2. 基本操作
   cout << "长度：" << s5.size() << endl;       // 获取长度：11                                 
   cout << "是否为空：" << s5.empty() << endl;  // 判断是否为空：0（false）                               
   cout << "第3个字符：" << s5[2] << endl;      // 访问字符：'l'（索引从0开始）
   // 3. 常用成员函数
   s2.append(" there");        // 追加字符串：s2 = "hello there"
   s2 += "!";                  // 运算符重载：s2 = "hello there!"
   size_t pos = s2.find("there");  // 查找子串位置：6
   if (pos != string::npos) {      // 判断是否找到（npos表示未找到）
   s2.replace(pos, 5, "world"); // 替换子串：s2 = "hello world!"
   }
    string sub = s2.substr(6, 5);   // 提取子串（从位置6开始，长度5）："world"
    // 4. 输入输出
    cout << "s2: " << s2 << endl;   // 输出：hello world!
    cout << "请输入字符串：";
    cin >> s1;                      // 输入字符串（遇空格结束）
    getline(cin, s1);               // 读取整行（包括空格）
    return 0;

}

```

#### 注意事项
1. **头文件**：必须包含 `<string>` 才能使用 `std::string`
2. **命名空间**：`string` 位于 `std` 命名空间中，需使用 `using namespace std;` 或 `std::string`
3. **比较运算**：支持 `==`、`!=`、`<`、`>` 等运算符，按字典序比较
4. **性能**：字符串频繁修改时，可考虑 `reserve(n)` 预分配空间提升效率

`std::string` 提供了丰富的成员函数用于字符串操作，以下是最常用的一些：

#### 常用成员函数
#### 1. 长度与容量相关
| 函数 | 功能 |
|------|------|
| `size()` / `length()` | 返回字符串长度（字符数量） |
| `empty()` | 判断字符串是否为空（长度为0返回`true`） |
| `reserve(n)` | 预分配至少能存储`n`个字符的空间（提升效率） |
| `capacity()` | 返回当前已分配的存储空间（可能大于实际长度） |
| `clear()` | 清空字符串（长度变为0，不释放内存） |

```cpp
string s = "hello";
cout << s.size();      // 输出：5
cout << s.empty();     // 输出：false
s.reserve(100);        // 预分配100个字符的空间
s.clear();             // s变为空字符串
```


#### 2. 字符串修改
| 函数 | 功能 |
|------|------|
| `append(str)` | 在末尾追加字符串`str` |
| `push_back(c)` | 在末尾追加单个字符`c` |
| `insert(pos, str)` | 在位置`pos`插入字符串`str` |
| `erase(pos, n)` | 从位置`pos`删除`n`个字符 |
| `replace(pos, n, str)` | 从位置`pos`开始，替换`n`个字符为`str` |
| `resize(n, c)` | 调整字符串长度为`n`，不足则用`c`填充 |

示例：
```cpp
string s = "abc";
s.append("def");       // s = "abcdef"
s.push_back('g');      // s = "abcdefg"
s.insert(3, "123");    // s = "abc123defg"
s.erase(3, 3);         // s = "abcdefg"（删除"123"）
s.replace(3, 3, "xyz");// s = "abcxyzg"
```
#### 3. 字符串查找
| 函数 | 功能 |
|------|------|
| `find(str, pos)` | 从位置`pos`开始查找`str`，返回首次出现的位置（未找到返回`string::npos`） |
| `rfind(str)` | 从末尾开始查找`str`，返回最后出现的位置 |
| `find_first_of(str)` | 查找`str`中任意字符首次出现的位置 |
| `find_last_of(str)` | 查找`str`中任意字符最后出现的位置 |

示例：
```cpp
string s = "hello world";
size_t pos = s.find("lo");  // pos = 3（"lo"从索引3开始）
if (pos != string::npos) {  // 检查是否找到
    cout << "找到位置：" << pos << endl;
}

size_t rpos = s.rfind("l"); // rpos = 9（最后一个'l'的位置）
```


#### 4. 子串与转换
| 函数 | 功能 |
|------|------|
| `substr(pos, n)` | 从位置`pos`开始，提取长度为`n`的子串（`n`省略则取到末尾） |
| `c_str()` | 转换为C风格字符串（`const char*`） |
| `data()` | 返回指向字符串数据的指针（与`c_str()`类似，C++11后功能一致） |

示例：
```cpp
string s = "hello world";
string sub = s.substr(6, 5);  // sub = "world"
const char* cstr = s.c_str(); // 用于需要C风格字符串的场景（如printf）
printf("%s\n", cstr);         // 输出：hello world
```


#### 5. 比较与赋值
| 函数 | 功能 |
|------|------|
| `compare(str)` | 与`str`比较（返回0表示相等，正数表示当前字符串更大） |
| `operator=` | 赋值运算符（直接赋值字符串） |
| `swap(s2)` | 与另一个字符串`s2`交换内容（高效，不涉及内存分配） |

示例：
```cpp
string s1 = "apple", s2 = "banana";
if (s1.compare(s2) < 0) {    // "apple" < "banana"，返回负数
    cout << s1 << " 小于 " << s2 << endl;
}

s1.swap(s2);                 // s1 = "banana", s2 = "apple"
```










### 函数system
在 C/C++ 中，`system` 是一个标准库函数，用于调用操作系统的命令行指令。它的原型定义在 `<stdlib.h>`（C 语言）或 `<cstdlib>`（C++）头文件中
```cpp
int system(const char* command);
```
- 功能说明：
- 接收一个字符串参数 `command`，该字符串是要在操作系统命令行中执行的指令
- 执行成功时，返回命令的退出状态；执行失败时，返回特定的错误值（通常是 -1）

- 常见用法示例：
1. **在 Windows 系统中**：
   ```cpp
   #include <cstdlib>  // 包含 system 函数的头文件
   
   int main() {
       system("dir");       // 列出当前目录文件（类似 Linux 的 ls）
       system("pause");     // 暂停程序，显示"按任意键继续..."
       system("notepad");   // 打开记事本程序
       return 0;
   }
   ```

2. **在 Linux/Unix 系统中**：
   ```cpp
   #include <cstdlib>
   
   int main() {
       system("ls");        // 列出当前目录文件
       system("sleep 3");   // 程序暂停 3 秒
       system("gedit");     // 打开文本编辑器（视系统配置而定）
       return 0;
   }
   ```


### 关键字new
`new` 是用于动态分配内存的运算符/运算符，主要作用是在**堆（heap）** 上创建对象或数组，并返回指向该内存的指针。它与 C 语言的 `malloc` 类似，但更符合 C++ 的面向对象特性（会自动调用构造函数）。
- 必须用delete释放内存，不可以用free
1. **创建单个对象**：
   ```cpp
   int* num = new int;  // 分配一个int类型的内存，返回指针
   *num = 10;           // 给分配的内存赋值
   
   // 创建对象时直接初始化
   int* num2 = new int(20);  // 分配int并初始化为20
   ```

2. **创建对象数组**：
   ```cpp
   int* arr = new int[5];  // 分配包含5个int的数组
   arr[0] = 1;             // 访问数组元素
   ```

3. **创建类对象**（会自动调用构造函数）：
   ```cpp
   class Person {
   public:
       Person() { cout << "构造函数被调用" << endl; }
   };
   
   Person* p = new Person();  // 分配Person对象，自动调用构造函数
   ```
- **必须释放内存**：用 `new` 分配的内存不会自动释放，需用 `delete`（单个对象）或 `delete[]`（数组）手动释放，否则会导致内存泄漏。
  ```cpp
  delete num;    // 释放单个对象
  delete[] arr;  // 释放数组（必须用delete[]，否则行为未定义）
  delete p;      // 释放类对象，会自动调用析构函数
  ```

- **与 `malloc` 的区别**：
  - `new` 是运算符，`malloc` 是函数
  - `new` 会自动计算所需内存大小，`malloc` 需要手动指定
  - `new` 会调用对象的构造函数，`delete` 会调用析构函数；`malloc`/`free` 仅分配/释放内存
  - `new` 失败时抛出 `bad_alloc` 异常（或返回空指针，取决于编译器设置），`malloc` 失败返回 `NULL`

- **避免野指针**：内存释放后，建议将指针置为 `nullptr`，防止访问已释放的内存：
  ```cpp
  delete num;
  num = nullptr;  // 避免野指针
  ```
***示例：***
```cpp
#include <iostream>
using namespace std;

int main() {
    // 动态分配单个整数
    int* a = new int(100);
    cout << *a << endl;  // 输出 100
    
    // 动态分配数组
    int* arr = new int[3]{1, 2, 3};  // C++11及以上支持初始化列表
    for (int i = 0; i < 3; i++) {
        cout << arr[i] << " ";  // 输出 1 2 3
    }
    
    // 释放内存
    delete a;
    delete[] arr;
    
    a = nullptr;
    arr = nullptr;
    
    return 0;
}
```

### 关键字delete
在 C++ 中，`delete` 是用于**释放动态分配的内存**的关键字，必须与 `new` 配对使用（`new` 分配内存，`delete` 回收内存），核心作用是避免内存泄漏。
- 栈对象不能用delete
**1. 基础用法：释放单个对象（`new` 对应 `delete`）**
```cpp
// 1. 动态分配单个Person对象（new调用构造函数）
Person* p = new Person(30); 
// 2. 使用对象
cout << p->age << endl; 
// 3. 释放内存（delete调用析构函数，回收内存）
delete p; 
p = nullptr; // 建议：释放后置空，避免野指针
```


**2. 释放数组（`new[]` 对应 `delete[]`）**
如果用 `new[]` 分配数组，必须用 `delete[]` 释放（不能用普通 `delete`）：
```cpp
// 动态分配Person数组
Person* arr = new Person[2]{Person(20), Person(30)}; 
// 使用数组
cout << arr[0].age << endl; 
// 释放数组（delete[] 会调用每个元素的析构函数）
delete[] arr; 
arr = nullptr;
```
⚠️ 错误示例：`delete arr;`（仅释放第一个元素，其余元素内存泄漏，析构函数也不会调用）。

**`delete` 的关键规则**
#### 1. 不能重复 `delete`，不能 `delete` 空指针？
- **重复 delete**：`delete p; delete p;` → 未定义行为（程序崩溃/内存错乱）；
- **delete nullptr**：合法且安全（编译器会直接忽略，无副作用），因此释放前判空是好习惯：
  ```cpp
  if (p != nullptr) {
      delete p;
      p = nullptr;
  }
  ```

#### 2. `delete` 只作用于指针，与引用无关
引用是对象的“别名”，不涉及动态内存分配，因此**不能对引用用 `delete`**：
```cpp
Person x1(30);
Person& ref = x1;
// delete ref; // ❌ 编译报错：引用不是指针，不能delete
```

**析构函数与 `delete`**
如果类包含动态分配的资源（比如指针成员），必须手动写析构函数，否则 `delete` 会导致内存泄漏：
```cpp
class Person {
public:
    int* data; // 动态分配的成员
    // 构造函数：分配内存
    Person() : data(new int(10)) {} 
    // 析构函数：释放成员的动态内存
    ~Person() {
        delete data; // 必须手动释放，否则delete Person对象时，data指向的内存泄漏
        data = nullptr;
    }
};

// 使用
Person* p = new Person();
delete p; // ① 调用~Person()释放data；② 释放p指向的Person内存
```
⚠️ 若不写析构函数：`delete p` 只会释放 `Person` 对象本身的内存，但 `data` 指向的堆内存永远无法回收（内存泄漏）。

**常见误区**
1. **对栈对象 `delete`**：
   ```cpp
   Person p(30); // 栈对象，编译器自动管理内存
   delete &p; // ❌ 崩溃！栈内存不能手动delete
   ```
2. **释放引用**：引用不是指针，`delete ref` 编译报错；
3. **混用 `delete` 和 `delete[]`**：`new[]` 必须用 `delete[]`，否则内存泄漏；
4. **释放后不置空**：导致野指针，后续误操作可能崩溃。


### 引用&
**引用（Reference）** 是一种特殊的变量类型，它为已存在的变量创建一个“别名”（alias）
- 格式：
类型 &引用名 = 原变量;
- 用const修饰别名，则别名不可修改，但原名可修改
```cpp
int a = 10;
int &ref = a;  // ref 是 a 的引用（别名）

ref = 20;      // 操作 ref 等同于操作 a
cout << a;     // 输出 20
```

1. **必须初始化**  
   引用在定义时必须绑定到一个已存在的变量，不能像指针一样先定义后赋值：
2. **一旦绑定，不可更改**  
   引用一旦与某个变量绑定，就不能再改为指向其他变量：
3. **没有空引用**  
   引用必须指向有效的变量，不存在“空引用”（这一点与指针不同，指针可以为 `nullptr`）。

1. **作为函数参数**  
   用于实现“传引用调用”，可以直接修改实参的值，避免值传递的拷贝开销：
   ```cpp
   void swap(int &x, int &y) {  // 引用参数
       int temp = x;
       x = y;
       y = temp;
   }
   
   int main() {
       int a = 1, b = 2;
       swap(a, b);  // 直接修改 a 和 b 的值
       cout << a << " " << b;  // 输出 2 1
       return 0;
   }
   ```
2. **作为函数返回值**  
   可以返回函数内静态变量或外部变量的引用，避免返回值的拷贝：
   ```cpp
   int &getMax(int &a, int &b) {
       return (a > b) ? a : b;
   }
   
   int main() {
       int x = 10, y = 20;
       getMax(x, y) = 30;  // 直接修改返回的引用（此处是 y）
       cout << y;  // 输出 30
       return 0;
   }
   ```


### this指针
- 核心作用是：指向当前调用成员函数的那个对象
**隐含存在**
- 所有非静态成员函数内部都隐式包含`this`指针（静态成员函数没有 this，因为不依赖对象）
**指针本质**
this 的类型是:类名* const,比如 Person* const,指向的对象地址不能改
**作用**
1. 区分重名的成员变量和参数
2. 返回当前对象本身（链式调用），函数类型必须是`类名&`

- PS：静态成员函数属于“类本身”，不依赖对象调用，没有隐含的this指针，因此不能用this
**区分形参和成员变量**
```cpp

class Person {
private:
    string name;
    int age;
public:
    // 构造函数：参数名和成员变量名重名（name/age）
    Person(string name, int age) {
        // this->name：当前对象的成员变量 name
        // name：构造函数的参数 name
        this->name = name; 
        this->age = age;  // 同理，区分成员变量和参数
    }
};
```
**返回当前对象本身（链式调用）**
```cpp
class Person {
private:
    int age;
public:
    // 返回 Person&（对象引用），才能链式调用
    //如果函数类型是Person，则返回的是一个拷贝的副本，此时链式调用修改的是副本而不是实例对象
    Person& setAge(int age) {
        this->age = age;
        return *this; // 返回当前对象本身（解引用 this 得到对象）
    }
    Person& addAge(int num) {
        this->age += num;
        return *this;
    }
    void showAge() {
        cout << "年龄：" << age << endl;
    }
};

int main() {
    // 链式调用：连续调用 setAge 和 addAge
    Person p;
    p.setAge(20).addAge(5).showAge(); // 输出：年龄：25
    return 0;
}
```
- 成员函数中调用成员变量，会隐式添加this指针。如果空指针调用成员函数，不会报错，但是如果成员函数用到成员变量，
- 就会触发this指针，导致报错


### 函数的默认值

设置默认值的语法是在函数声明或定义时，在参数后加 `= 默认值`：
调用时可以省略带默认值的参数：
```cpp
printInfo("张三");                  // 使用 age=18，gender="未知"
printInfo("李四", 20);              // 使用 gender="未知"
printInfo("王五", 22, "男");        // 不使用默认值
```
1. **默认值必须从右向左设置**  
   一旦某个参数设置了默认值，它右边的所有参数都必须设置默认值（不能跳过某个参数给后面的参数设默认值）：
   ```cpp
2. **默认值只能在声明或定义中指定一次**  
   通常在函数声明（头文件中）指定默认值，定义时不再重复，避免不一致：
   ```cpp
   // 头文件中声明（推荐在这里设默认值）
   void func(int x = 5);
   
   // 源文件中定义（不要重复默认值）
   void func(int x) { ... }
   ```
3. **默认值可以是常量、全局变量或表达式**  
4. **函数重载时需注意歧义**  
   如果重载函数的参数列表与默认值组合可能导致调用歧义，编译器会报错





### 函数重载
**函数重载** 是指在同一作用域内，可以可以定义多个同名函数，只要它们的**参数列表不同**（参数个数、类型或顺序不同）。编译器会根据调用时提供的实参自动匹配到对应的函数版本。

函数重载的判定依据是**参数列表（形参的数量、类型或顺序）**，与返回值类型无关。

- 参数个数不同
```cpp
void print(int x)
void print(int x, int y)   // 参数个数不同，构成重载
```
- 参数类型不同
```cpp
void print(int x)
void print(double x)   // 参数类型不同，构成重载
```
- 参数顺序不同（当类型不同时）
```cpp
void print(int x, double y) {
    cout << "int, double: " << x << ", " << y << endl;
}

void print(double x, int y) {  // 参数顺序不同，构成重载
    cout << "double, int: " << x << ", " << y << endl;
}
```

1. **返回值类型不能作为重载依据**  
   仅返回值不同的函数不能构成重载，编译器会报错：

2. **默认参数可能导致歧义**  
   当重载函数与默认参数结合时，可能产生调用歧义：
   ```cpp
   void func(int a) { ... }
   void func(int a, int b = 10) { ... }
   
   func(5);  // 歧义：不知道调用哪个版本（编译器报错）
   ```

3. **引用与const引用可以构成重载**  
   区分普通引用和const引用：
   ```cpp
   void func(int &x) { cout << "非const引用" << endl; }
   void func(const int &x) { cout << "const引用" << endl; }
   
   int main() {
       int a = 10;
       const int b = 20;
       func(a);  // 调用第一个版本（非const引用）
       func(b);  // 调用第二个版本（const引用）
       func(30); // 调用第二个版本（字面量是const，匹配const引用）
   }
   ```


### 关键字operator（运算符重载）
允许我们为`自定义类型`重新定义`运算符`（如 +、==、[]、<<）的行为，
让运算符能像操作内置类型（int、double）一样操作自定义对象
例如：让编译器知道两个Person对象进行加法运算，会发生什么

**本质** 运算符重载本质是`函数重载`，编译器会将运算符表达式（如 a + b）转换为对应重载函数的调用
#### 规则

1. 不能创造新运算符：只能重载已有的运算符如 +、-、*、/、==、[]、() 等不能自定义新运算符（如 #、@）

2. 运算符优先级/结合性不变：重载后运算符的优先级、结合性、操作数个数仍和原运算符一致（如 + 仍低于 *，左结合）

3. 不能重载的运算符（5 个）：
`.`（成员访问运算符）
`.*`（成员指针访问运算符）
`::`（作用域解析运算符）
`sizeof`（大小运算符）
`?:`（三目条件运算符）
#### 左右操作数
对于二元运算符（比如 +、-、*、/），格式都是 左操作数 运算符 右操作数
```cpp
a + b;  // a = 左操作数（LHS），b = 右操作数（RHS）
p1 + p2; // p1 = 左操作数，p2 = 右操作数
10 + p;  // 10 = 左操作数，p = 右操作数
```
- 若左操作数是当前类的对象（比如 Person p1; p1 + 其他）→ 用「成员函数版」重载；
- 若左操作数不是当前类的对象（比如 int 10 + Person p、cout << Person p）→ 必须「全局函数版」重载

**格式**
```cpp
// 成员函数形式（左操作数是当前对象）
//如p.operator+(p2)
返回值类型 operator运算符(参数列表) {
    // 运算符逻辑
}

// 全局函数形式（需通过参数传入两个操作数，常配合友元）
返回值类型 operator运算符(操作数1类型 左操作数, 操作数2类型 右操作数) {
    // 运算符逻辑
}
```
- 成员函数重载运算符优先级高于全局函数重载运算符
- 如果两个同时存在，编译器会优先匹配成员函数

### 类型
**加法运算符重载+**
**左移运算符重载<<**
`p.operator<<(cout)`等价于`p`<<cout`
不符合`cout<<p`,因此一般不会使用成员函数重载左移运算符,只能理由全局函数重载左移运算符
```cpp
//一般在类中用友元声明friend ostream operator<<(ostream& cout, const Person& p);
ostream operator<<(ostream& cout, const Person& p)//本质 opperator<<(cout, p)，等价于cout << p
{
    cout << "(" << p.x << "," << p.y << ")" ;
    return cout;//是为了链式调用，即cout << p后还能继续输出<< endl;
}
```
**递增运算符重载++**
分为前置递增和后置递增，a++ ++a
```cpp
//前置递增
Person& operator++() {//++p等价于p.operator++();
    age++;
    return *this;//返回对象本身，减少拷贝
}
//后置递增
//返回的是值不是引用，因为temp是局部对象
//p++等价于p.operator++(0);
Person operator++(int) {//int用于重载函数，区分前置后置
    Person temp = *this;//先记录当前值
    age++;//再递增
    return temp;//返回记录的当前值
}
```
**赋值运算符重载=**
编译器会提供默认的赋值运算符重载函数，但是是浅拷贝，这样在析构函数中释放资源时会重复导致错误，
为了实现深拷贝，必须自定义赋值运算符重载函数
```cpp
Person& operator=(const Person& p) {
    if (this != &p) { // 自赋值检查
        if (age != NULL) {//查看是否已分配内存
            delete age;//释放内存
            age = NULL;//重置为NULL
        }
        age = new int(*p.age);//深拷贝
    }
    return *this;//返回当前对象的引用，支持链式赋值
}
```
**关系运算符重载**
**函数调用运算符重载**

**1.成员函数重载运算符**
- 一个参数
```cpp
class Person {
private:
    int x, y;
public:
    Person() {};
    Person(int x, int y) 
    {
        this->x = x;
        this->y = y;
    }
    void show() {
        cout << "(" << x << "," << y << ")" << endl;
    }
    // 1. 成员函数形式重载
    Person operator+(const Person& p) const {
        // 返回新对象
        Person temp;
        temp.x = this->x + p.x;
        temp.y = this->y + p.y;
        return temp;
    }
    // 友元声明：为了演示全局函数形式
    friend Person operator+(const Person& p1, const Person& p2);
};
int main() {
    Person p1(1, 2), p2(3, 4);
    Person p3 = p1 + p2; // 等价于 p1.operator+(p2)（成员函数形式）
    p3.show(); // 输出 (4, 6)
    return 0;
}
```

**2.全局函数重载运算符**
- 两个参数
```cpp
// 全局函数形式重载 +（需访问私有成员 x/y，因此声明为友元）
Person operator+(const Person& p1, const Person& p2) {
    Person temp;
    temp.x = p1.x + p2.x;
    temp.y = p1.y + p2.y;
    return temp;
}
int main() {
    Person p3 = p1 + p2; // 等价于 p1.operator+(p2)（成员函数形式）或 operator+(p1,p2)（全局函数形式）
    p3.show(); // 输出 (4, 6)
    return 0;
}
```




### 关键字friend（友元）
- 允许类外部的函数、其他类，或者其他类的成员函数，直接访问当前类的`private`成员和`protected`成员

**1.普通函数作为友元**
- 友元函数不属于任何类，无`this`指针
- 访问类成员时，必须通过函数参数传入的对象（或对象指针/引用）
- 必须用`friend`在类内声明
```cpp
class Person {
private:
    string name;
    int age;

    // 声明全局函数 printPerson 为友元
    friend void printPerson(const Person& p);

public:
    Person(string n, int a) : name(n), age(a) {}
};

// 友元函数：在类外部定义，可直接访问 Person 的 private 成员
void printPerson(const Person& p) {
    // 直接访问私有成员 name 和 age，无需 getter
    cout << "姓名：" << p.name << "，年龄：" << p.age << endl;
}

int main() {
    Person p("张三", 20);
    printPerson(p); // 输出：姓名：张三，年龄：20（友元函数直接访问私有成员）
    return 0;
}
```
**2.类作为友元**
- 友元关系是单向的：A声明B为友元，不代表B自动声明A为友元
- 友元关系不可传递：A是B的友元，B是C的友元，不代表A是C的友元
- 友元关系不可继承：父类的友元，不会自动成为子类的友元
- 必须用`friend`在类内声明友元类
```cpp
class Person {
private:
    string name;
    int age;
    // 声明 FriendClass 为友元类（FriendClass 的所有成员函数都能访问 Person 的私有成员）
    friend class FriendClass;
public:
    Person(string n, int a) : name(n), age(a) {}
};
// 友元类：可访问 Person 的私有成员
class FriendClass {
public:
    void showPerson1(const Person& p) {
        cout << "showPerson1：" << p.name << "，" << p.age << endl;
    }
};

int main() {
    Person p("李四", 25);
    FriendClass fc;
    fc.showPerson1(p); // 输出：showPerson1：李四，25
    return 0;
}
```

**3.其他类的特定成员函数作为友元**
- 必须先做前向声明：如果友元成员函数所属的类（如 FriendClass）需要用到当前类（如Person）作为参数，必须在定义该类前声明`class Person;`
- 声明顺序有要求：先定义友元成员函数所属的类（仅声明函数，不定义）,再定义当前类并声明友元，最后定义友元成员函数
```cpp
class Person; //0.前向声明：告诉编译器Person是一个类（后续会定义）

//1.先定义友元成员函数所属的类
class FriendClass {
public:
    // 声明成员函数（参数需要Person类型，因此需要前向声明）
    void showPerson(const Person& p);
};
// 定义Person类，声明FriendClass::showPerson为友元
class Person {
private:
    string name;
    int age;
    // 声明FriendClass的showPerson成员函数为友元（仅这一个函数有权限）
    friend void FriendClass::showPerson(const Person& p);
public:
    Person(string n, int a) : name(n), age(a) {}
};
// 定义友元成员函数：此时 Person 已完整定义，可访问其私有成员
void FriendClass::showPerson(const Person& p) {
    cout << "友元成员函数：" << p.name << "，" << p.age << endl;
}
int main() {
    Person p("王五", 30);
    FriendClass fc;
    fc.showPerson(p); // 输出：友元成员函数：王五，30
    return 0;
}
```



### 封装

**意义**：将数据（成员变量）和操作数据的方法（成员函数）捆绑在一起，并对外部隐藏对象的内部实现细节，只通过公共接口与对象进行交互


**结构**
class 类名
{
   权限控制符：
   属性（变量）；
   行为（函数）；
}
- 权限控制符为空默认是私有权限`private`
**定义**
类名 对象;
**使用**
对象.成员函数;/对象.成员变量;


**访问权限**：
1. **`private`（私有成员）**：
   - 只能在类内部被访问（成员函数可访问）。
   - 外部代码（包括派生类）无法直接访问。
   - 通常用于存储对象的核心数据（成员变量）。
2. **`public`（公有成员）**：
   - 可以被类外部的任何代码访问。
   - 通常用于定义类的接口（成员函数），供外部调用以操作私有数据。
3. **`protected`（保护成员）**：
   - 类内部可访问，派生类也可访问，但类外部不可访问。
   - 主要用于继承场景，平衡封装性和扩展性。

- 函数不存储在类中，不占用类的空间
#### 封装示例代码
下面是一个体现封装特性的学生类（`Student`）示例：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    // 私有成员：数据被隐藏，外部无法直接修改
    string name;  // 姓名
    int age;      // 年龄
    float score;  // 分数

public:
    // 公有成员函数：提供接口供外部操作私有数据
    // 设置姓名（带合法性检查）
    void setName(string n) {
        if (!n.empty()) {  // 确保姓名不为空
            name = n;
        }
    }

    // 获取姓名
    string getName() {
        return name;
    }

    // 设置年龄（带范围检查）
    void setAge(int a) {
        if (a >= 0 && a <= 120) {  // 确保年龄合理
            age = a;
        }
    }

    // 获取年龄
    int getAge() {
        return age;
    }

    // 设置分数（带范围检查）
    void setScore(float s) {
        if (s >= 0 && s <= 100) {  // 确保分数在0-100之间
            score = s;
        }
    }

    // 获取分数
    float getScore() {
        return score;
    }

    // 显示学生信息
    void showInfo() {
        cout << "姓名：" << name << endl;
        cout << "年龄：" << age << endl;
        cout << "分数：" << score << endl;
    }
};//别忘记分号

int main() {
    Student stu;//定义一个Student类的对象stu

    // 通过公有接口操作私有数据（符合封装原则）
    stu.setName("张三");
    stu.setAge(20);
    stu.setScore(95.5);
    stu.showInfo();

    // 尝试直接访问私有成员（编译报错，体现封装的保护作用）
    // stu.name = "李四";  // 错误：'name'是私有成员
    // stu.age = -5;       // 错误：'age'是私有成员

    return 0;
}
```



### 继承

### 多态




### 构造函数
**构造函数**和**析构函数**是类的两个特殊成员函数，分别负责**对象的初始化**和**对象销毁时的资源清理**，二者均由编译器自动调用，`无需手动触发`

构造函数的核心作用是：**在对象创建时，初始化对象的成员变量（如分配内存、设置初始值），确保对象从创建开始就是“可用状态”**。


#### 结构
`类名(){}`

**基本特性**
- **函数名**：与类名**完全相同**（大小写敏感），无返回值（连 `void` 都`不能`写）。
- **调用时机**：在对象被创建的瞬间自动调用（如 `Student stu;` 或 `new Student()` 时），且**仅调用一次**。

**嵌套类对象**
- 构造函数：嵌套内部的类对象先触发，外部的后触发
- 析构函数：嵌套外部的类对象先触发，内部的后触发

#### 2. 常见分类与示例

| 类型               | 特点                                  | 示例代码                                                                 |
|--------------------|---------------------------------------|--------------------------------------------------------------------------|
| 默认构造函数       | 无参数                                | `Student() { name = ""; age = 0; score = 0.0; }`                         |
| 有参构造函数       | 带参数，支持自定义初始化              | `Student(string n, int a, float s) { name = n; age = a; score = s; }`    |
| 拷贝构造函数       | 参数为“同类对象的引用”，用于对象拷贝  | `Student(const Student& other) { name = other.name; age = other.age; }`  |
| 委托构造函数       | 复用其他构造函数的逻辑（C++11 特性）  | `Student() : Student("", 0, 0.0) {}`（委托有参构造初始化）               |

- 若用户定义了有参构造函数，则C++会提供默认拷贝构造，但不会提供无参构造
    - 因此如果使用无参的方法构造对象，则会报错，必须先定义默认构造函数
- 若用户定义了拷贝构造函数，则C++不会提供无参构造和有参构造函数
    - 因此如果使用无参/有参的方法构造对象，则会报错，必须先定义默认构造函数/有参构造函数

**浅拷贝**
- 编译器自动生成的默认拷贝构造函数就是浅拷贝。
- 当类中没有指针成员（或不需要管理资源）时，浅拷贝是安全的。
- 浅拷贝会复制成员变量的值，对于指针，会指向同一内存地址(安全问题)
- 例如拷贝后，修改新对象指针，源对象也会被修改。
- 通过析构函数delete释放指针时，会重复释放导致内存泄漏或程序崩溃


**深拷贝**
- 不仅复制指针变量本身，还会复制指针指向的资源（如重新申请一块内存，将原资源的值拷贝到新内存）。
- 拷贝后，原对象和新对象的指针成员指向不同的内存块，但内容相同。
- 必须手动管理堆资源


| 对比项       | 浅拷贝（默认拷贝构造）                | 深拷贝（手动实现）                  |
|--------------|---------------------------------------|-------------------------------------|
| 拷贝内容     | 只复制指针的地址，不复制资源          | 复制指针指向的资源，重新申请内存    |
| 指针指向     | 原对象和新对象的指针指向同一块内存    | 原对象和新对象的指针指向不同内存    |
| 安全性       | 包含指针成员时，析构会导致重复释放    | 包含指针成员时，安全释放资源        |
| 使用场景     | 类中无指针/资源成员时使用             | 类中有指针/动态资源时必须使用       |


#### 3. 初始化列表（推荐用法）
构造函数中初始化成员变量时，**推荐使用“初始化列表”**，而非在函数体中赋值。初始化列表在对象内存分配时直接初始化成员，效率更高（尤其对类类型成员），且某些场景下必须使用（如 `const` 成员、引用成员）。
引用成员（int& b;）：引用必须在创建时绑定对象，只能用初始化列表。
**结构**：构造函数():成员变量1(值1),成员变量2(值2),成员变量3(值3)...{函数体}
**示例1**：
```cpp
class Student {
private:
    string name;  // 类类型成员
    const int age; // const 成员（必须在初始化列表初始化）
    float score;

public:
    // 初始化列表：成员变量按“类中声明顺序”初始化（上面的），而非列表顺序（下面的）
    //也就是说依次初始化name、age、score，而不是按照下面的age、name、score
    //此时如果初始化列表是age(a), name(age),score(s)，则name会是随机数，因为age还没初始化
    Student(string n, int a, float s) :  age(a), name(n),score(s) {
        // 函数体可后续处理逻辑（如参数合法性检查）
        xxxxxxxxxxxx
    }
};
```
**完整示例**
```cpp
#include <iostream>
using namespace std;
class Person
{

public:

    int age;
    Person()
    {
        cout <<"默认构造函数"<<endl;
    }
    Person(int a)
    {
        age=a;
        cout <<"有参构造函数"<<endl;
        cout <<"age="<<age<<endl;
    }
    Person(const Person &p)
    {
        age = p.age;
        cout <<"拷贝构造函数"<<endl;
        cout <<"age="<<age<<endl;
    }

};


//通过值传递给函数传参

void test1(Person p)
{
    cout <<"拷贝构造函数传递参数值"<<endl;
    cout <<"age="<<p.age<<endl;
}
void test0()
{
    Person x1(30);
    test1(x1);
}




//值方式返回局部对象
Person test2()
{
    Person x2;
    return x2;
}
void test3()
{
    Person x3=test2();
}
int main()
{
    //调用无参构造函数
    Person p1;
    //调用有参构造函数
    Person p2(20);
    //调用拷贝构造函数
    Person p3(p2);

    //拷贝函数的应用场景
    //1.用已创建的对象初始化新对象
    Person p4(p2);
    //2.通过值传递给函数传参
    test0();
    //3.值方式返回局部对象
    test3();
    cout<<"程序执行到这了"<<endl;
    system("pause");
}


```


### 析构函数
析构函数的核心作用是：**在对象销毁时（如离开作用域、`delete` 动态对象），释放对象占用的资源（如堆内存、文件句柄、网络连接），避免内存泄漏**。


#### 1. 基本特性
- **函数名**：`~` + 类名（如 `~Student()`），无返回值，无参数（不能重载）。
- **调用时机**：对象生命周期结束时自动调用（如局部对象离开作用域、动态对象被 `delete`），且**仅调用一次**。
- **默认行为**：若类中未显式定义析构函数，编译器会生成“默认析构函数”，但仅能清理类的成员变量（如 `std::string` 会调用自身析构释放内存），无法处理“自定义资源”（如堆内存）。

#### 结构
`~类名(){}`
- 比构造函数多一个`~`


#### 2. 典型使用场景：释放自定义资源
当类的成员变量包含**动态分配的资源**（如 `new` 分配的数组、指针）时，必须显式定义析构函数，否则会导致内存泄漏。

**示例：处理动态内存**
```cpp
class MyArray {
private:
    int* arr;   // 动态分配的数组指针
    int size;

public:
    // 构造函数：分配堆内存
    MyArray(int s) : size(s) {
        arr = new int[size]; // 分配 size 个 int 的内存
        for (int i = 0; i < size; i++) {
            arr[i] = i; // 初始化数组
        }
    }

    // 析构函数：释放堆内存（必须显式定义）
    ~MyArray() {
        delete[] arr; // 释放数组内存（注意用 delete[] 对应 new[]）
        arr = nullptr; // 避免野指针
    }

    // 其他成员函数（如打印数组）
    void print() {
        for (int i = 0; i < size; i++) {
            cout << arr[i] << " ";
        }
    }
};

// 使用示例
int main() {
    MyArray arr(5); // 创建对象，调用构造函数（分配内存）
    arr.print();    // 输出：0 1 2 3 4
    return 0;       // 函数结束，arr 离开作用域，自动调用析构函数（释放内存）
}
```

