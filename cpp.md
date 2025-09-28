
### 命名空间
C++ 标准库（比如 `cout`、`cin`、`endl` 这些工具）里的所有内容，都被放在一个名为 `std` 的“命名空间”中。  
命名空间的作用是**避免名字冲突**——比如，你自己写的代码里可能有一个叫 `cout` 的变量，这就会和标准库的 `cout` 重名，而命名空间可以区分它们。

- `using namespace std;` 的作用
这句话的意思是：**“我要使用 `std` 这个命名空间里的所有内容”**。  
加上这句话后，你可以直接写 `cout`、`cin`、`endl` 等，不用额外说明它们来自 `std` 命名空间。

- 如果不写 `using namespace std;`，就必须在标准库的工具前加上 `std::` 前缀，明确说明“这个工具来自 `std` 命名空间”。  


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

输入示例：  
`10 20 3.14`  
输出结果：  
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


### 运算符new
`new` 是用于动态分配内存的运算符，主要作用是在**堆（heap）** 上创建对象或数组，并返回指向该内存的指针。它与 C 语言的 `malloc` 类似，但更符合 C++ 的面向对象特性（会自动调用构造函数）。
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

### 引用
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
