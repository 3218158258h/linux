## C++基础
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


---
### 标准输出
```c++
cout << 数据
```
- cout << 1 << 2 << 3;     结果: 123
- cout << endl;     结果: 换行
- cout << 1 << 2 << endl;    结果: 12后换行
- cout << "hello world";     结果: hello world

---
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

---
### 左值和右值
**右值引用** 是 C++11 引入的核心特性之一，是实现**移动语义**和**完美转发**的基础。它的语法是 `T&&`，但与传统的“左值引用” `T&` 有本质区别。

#### 一、什么是左值和右值？
理解右值引用的前提是区分 **左值** 和 **右值**：
| 类型 | 特点 | 示例 |
|------|------|------|
| **左值（lvalue）** | 有名字、可取地址、生命周期较长 | 变量 `int x = 5;` 中的 `x` |
| **右值（rvalue）** | 临时对象、无名字、即将销毁 | 字面量 `42`、函数返回的临时对象 `func()` |

 - **左值** = 能出现在赋值号左边的东西（如变量）  
 - **右值** = 只能出现在右边的东西（如字面量、临时值）

C++11进一步将右值细分为：
- **纯右值（prvalue）**：如 `5`, `func()`（返回非引用）
- **将亡值（xvalue, eXpiring value）**：如 `std::move(x)` 的结果


#### 二、右值引用的定义与绑定规则

 1. 语法
```cpp
T&& rref = /* 必须是一个右值 */;
```
#### 2. 绑定规则（关键！）
| 引用类型       | 可绑定到左值？ | 可绑定到右值？ |
|----------------|----------------|----------------|
| `T&`           | ✅             | ❌             |
| `const T&`     | ✅             | ✅（经典技巧） |
| `T&&`          | ❌             | ✅             |
| `const T&&`    | ❌             | ✅（少见）     |

✅ 正确示例：
```cpp
int&& a = 42;               // OK：42 是右值
int&& b = int(10);          // OK：临时对象
int x = 5;
int&& c = std::move(x);     // OK：std::move 把 x 转为右值引用（xvalue）
```

❌ 错误示例：
```cpp
int x = 5;
int&& d = x;                // 编译错误！x 是左值，不能绑定到 T&&
```

---

#### 三、`std::move` 的作用

`std::move` **不移动任何东西**！它只是一个**类型转换工具**：

```cpp
template<typename T>
typename std::remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}
```

- 它把**左值**强制转换成**右值引用**（xvalue），从而可以触发移动构造或移动赋值。
- 它只是“允许你移动”，实际是否移动取决于是否有对应的移动函数。

#### 示例：
```cpp
std::vector<int> v1 = {1, 2, 3};
std::vector<int> v2 = std::move(v1); // v1 的内部指针被“偷走”
// 此时 v1 处于有效但未指定状态（通常为空）
```

> ⚠️ 注意：`std::move` 后不要使用原对象，除非你知道它的状态！

---

#### 四、右值引用的典型用途

##### 1. 实现移动构造函数 / 移动赋值运算符
```cpp
class Widget {
    Resource* ptr;
public:
    // 移动构造函数
    Widget(Widget&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr; // 防止双重释放
    }
};
```

##### 2. 支持高效容器操作
```cpp
std::vector<std::string> vec;
vec.push_back("hello");           // 临时字符串 → 触发移动
std::string s = "world";
vec.push_back(std::move(s));      // 显式移动 s
```

##### 3. 完美转发（结合模板）
```cpp
template<typename T>
void wrapper(T&& arg) {
    foo(std::forward<T>(arg)); // 保持原始值类别（左值/右值）
}
```
> 这里 `T&&` 是**转发引用（universal reference）**，不是普通右值引用！



#### 五、常见误区

| 误区 | 正确理解 |
|------|----------|
| “右值引用只能绑定临时对象” | 也能绑定 `std::move(x)` 的结果（xvalue） |
| “`std::move` 会立即移动数据” | 它只是类型转换，移动由构造函数/赋值符完成 |
| “右值引用本身是右值” | **命名的右值引用是左值！**例如：`void f(int&& x) { /* x 是左值 */ }` |

> 🔥 关键点：**在函数体内，`x` 是一个有名字的变量 → 是左值！**  
> 若想把它作为右值传递，必须再套一层 `std::move(x)` 或 `std::forward<T>(x)`。



#### 六、总结

- **右值引用 `T&&`**：只能绑定到右值（临时对象或 `std::move` 的结果）。
- **核心价值**：实现移动语义，避免不必要的拷贝，提升性能。
- **配合使用**：`std::move`（显式转右值）、移动构造函数、移动赋值运算符。
- **注意**：移动后源对象应处于可析构状态（通常置空）。


---
### 关键字const和constexpr
#### const
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
#### constexpr
`constexpr` 是 C++11 引入的关键字，用于**声明可在编译时求值的常量表达式**。它的核心目标是将计算从运行时移到编译时，提升性能、支持模板元编程，并增强类型安全。
**一、基本作用**
`constexpr` 可用于修饰：
- **变量**
- **函数（包括构造函数）**
- **对象**
前提是它们的值或行为**必须能在编译期确定**。

**二、`constexpr` 变量**
```cpp
constexpr int x = 42;               // ✅ 合法：字面量初始化
constexpr int y = x + 10;           // ✅ 合法：由 constexpr 表达式初始化
constexpr int z = rand();           // ❌ 非法：rand() 在运行时才能调用
```
> 要求：  
> - 初始化表达式必须是**常量表达式**（编译期可计算）  
> - 类型必须是**字面类型（LiteralType）**（如 int、指针、简单 struct 等）

**三、`constexpr` 函数**

```cpp
constexpr int square(int n) {
    return n * n;
}
int main() {
    constexpr int a = square(5);    // ✅ 编译时计算：a = 25
    int b = square(6);              // ✅ 也可在运行时调用
}
```

**C++11 限制（较严格）**    
- 函数体只能包含 `return` 语句（不能有循环、局部变量等）
- 所有参数和返回值必须是字面类型

**C++14/17/20 放宽**
- 允许**循环、条件语句、局部变量、多条语句**等
- 示例（C++14 起合法）：
  ```cpp
  constexpr int factorial(int n) {
      int result = 1;
      for (int i = 2; i <= n; ++i)
          result *= i;
      return result;
  }
  ```
> ⚠️ 注意：`constexpr` 函数**不一定在编译期执行**！  
> - 如果传入的是运行时常量，则在运行时计算  
> - 只有当所有参数都是编译期常量，且结果被用于需要常量表达式的上下文（如数组大小、模板参数），才会强制编译期求值。


**四、`constexpr` 对象与构造函数**

```cpp
struct Point {
    constexpr Point(int x, int y) : x(x), y(y) {}
    constexpr int distance_sq() const {
        return x * x + y * y;
    }
    int x, y;
};

constexpr Point p(3, 4);            // ✅ 编译期构造
constexpr int d = p.distance_sq();  // ✅ 编译期计算：25
```
要求：
- 成员变量必须是字面类型
- 构造函数必须是 `constexpr`
- 所有成员初始化必须在编译期完成

**五、典型应用场景**
1. **编译期计算**  
   ```cpp
   constexpr int N = 1024;
   std::array<int, N> buf;  // 数组大小必须是编译期常量
   ```

2. **模板元编程**  
   ```cpp
   template<int N>
   struct Factorial {
       static constexpr int value = N * Factorial<N-1>::value;
   };
   ```

3. **替代宏**（更安全）  
   ```cpp
   #define MAX_SIZE 100   // 宏，无类型检查，作用域全局
   constexpr int max_size = 100; // 类型安全，作用域清晰
   ```
4. **性能优化**  
   将重复计算移到编译期，减少运行时开销。   
#### const与constexpr的区别
1. **可用于必须使用编译期常量的场合**
这是最核心的区别。

| 场景 | `const` | `constexpr` |
|------|--------|-------------|
| 数组大小 | ❌（C++中非法） | ✅ |
| 模板非类型参数 | ❌ | ✅ |
| 枚举值初始化 | ❌ | ✅ |
| `switch` 的 case 标签 | ❌ | ✅ |

```cpp
const int n = 10;
int arr[n];  // ❌ 错误！C++ 中数组大小必须是编译期常量（C99 允许，但 C++ 不允许）

constexpr int m = 10;
int arr2[m]; // ✅ 合法：m 是编译期常量
```

```cpp
template<int N>
struct Buffer { char data[N]; };

const int size = 5;
Buffer<size> buf; // ❌ 错误：size 不是编译期常量

constexpr int size2 = 5;
Buffer<size2> buf2; // ✅ 合法
```
2. **保证在编译期计算，提升性能**
- `constexpr` 表达式**一定在编译期求值**（当用于常量上下文时），避免运行时开销。
- `const` 变量可能在运行时初始化，无法用于优化。

```cpp
const int x = some_runtime_function(); // 运行时计算，无法优化
constexpr int y = 2 + 3 * 4;           // 编译器直接替换为 14，零运行时成本
```

3. **支持编译期函数和对象构造**
`constexpr` 可修饰函数和类构造函数，实现**复杂的编译期逻辑**，而 `const` 不能。

```cpp
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

constexpr int f = factorial(5); // 编译期计算出 120

// const 无法做到这一点：
// const int f2 = factorial(5); // 虽然能运行，但不保证编译期求值，
//                              // 且不能用于数组大小等场景
```

**使用建议**
- **定义常量时，优先用 `constexpr` 而不是 `const`**（只要值能在编译期确定）。
- 例如：
  ```cpp
  // 好
  constexpr double PI = 3.1415926535;
  constexpr size_t MAX_THREADS = 8;

  // 避免（除非值只能在运行时获得）
  const double config_value = read_from_file(); // 此时只能用 const
  ```
> ✅ **记住**：  
> - `const` → “运行时不变”  
> - `constexpr` → “编译期已知 + 不变”  

---
### 关键字static
- 定义在函数内部则作用域在内部，每次调用不会清零
- 作用域在cpp文件则作用域限定在本文件
- 定义在类内则作用域为类，必须在类外初始化，每个对象对于同一个staic共享数据，不同static数据不共享
- 所有对象的同一静态成员变量共享数据，不同静态成员变量不共享数据
- 类中定义的静态成员变量在编译阶段分配内存;必须类外初始化，私有数据不影响初始化，但是影响读写
- 所有对象共享同一静态成员函数
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
---
### 关键字extern

| 概念       | 含义                                                                 |
|------------|----------------------------------------------------------------------|
| **定义**   | 为变量分配内存（或为函数提供实现），且只能有一次（否则会报“重复定义”） |
| **声明**   | 告诉编译器变量/函数的类型和名称，不分配内存，可多次声明               |

- `extern` 的核心就是**仅做声明，不做定义**，它的使命是“跨文件/跨作用域引用已定义的变量/函数”。


#### 场景1：跨文件共享全局变量（最常用）
假设你有两个源文件：`main.cpp` 和 `utils.cpp`，想在 `main.cpp` 中使用 `utils.cpp` 里定义的全局变量。

✅ 正确写法：
```cpp
// utils.cpp —— 定义全局变量（分配内存）
int g_num = 100; // 定义：分配内存，初始化值为100
```
```cpp
// main.cpp —— 声明并使用外部变量
// extern声明：告诉编译器g_num的定义在其他文件中
extern int g_num; 
int main() {
    cout << g_num << endl; // 输出：100（访问utils.cpp中的g_num）
    g_num = 200; // 可以修改（因为只是声明，变量本身在utils.cpp中已定义）
    cout << g_num << endl; // 输出：200
    return 0;
}
```


#### 场景2：跨文件共享函数
函数默认具有“外部链接性”，因此 `extern` 修饰函数是可选的，但显式写 `extern` 能让代码更清晰：
```cpp
// utils.cpp —— 定义函数（提供实现）
int add(int a, int b) {
    return a + b;
}
```
```cpp
// main.cpp —— 声明并使用外部函数
// 方式1：显式extern声明函数（推荐，语义清晰）
extern int add(int a, int b); 

// 方式2：省略extern（函数默认外部链接，等价于上面）
// int add(int a, int b);
int main() {
    cout << add(3, 5) << endl; // 输出：8
    return 0;
}
```

#### 场景3：`extern "C"` —— 兼容C语言编译（重要）
C++编译器会对函数名做“名字修饰（name mangling）”（比如把 `add(int, int)` 改成 `_Z3addii`），而C编译器不会。如果想在C++代码中调用C语言编译的函数（比如C写的库），需要用 `extern "C"` 告诉C++编译器：“按C语言的规则处理这个函数”。

示例：
```cpp
// 头文件 utils.h（供C和C++共用）
#ifdef __cplusplus
extern "C" { // 告诉C++编译器，花括号内按C规则编译
#endif
int add(int a, int b); // C风格函数声明
#ifdef __cplusplus
}
#endif
```

```cpp
// utils.c —— C语言实现
#include "utils.h"
int add(int a, int b) {
    return a + b;
}
```

```cpp
// main.cpp —— C++调用C函数
#include <iostream>
#include "utils.h"
using namespace std;

int main() {
    cout << add(3, 5) << endl; // 正确调用C语言编译的add函数
    return 0;
}
```
#### 场景4：声明全局作用域的变量（局部作用域中）
如果局部作用域中有和全局变量同名的变量，想访问全局变量，可通过 `extern` 声明：
```cpp
int num = 10; // 全局变量

int main() {
    int num = 20; // 局部变量（覆盖全局）
    cout << num << endl; // 输出：20（局部变量）
    
    extern int num; // 声明：引用全局的num
    cout << num << endl; // 输出：10（全局变量）
    return 0;
}
```
#### `extern` 的常见误区
1. **`extern` 声明时不要初始化**：
   `extern int g_num = 100;` 不是纯声明，而是**定义**（编译器会分配内存），如果多个文件写这个，会报重复定义错误。
2. **`extern` 不改变变量的存储类型**：
   它只负责“链接性”，不影响变量是全局/静态/局部（比如 `static int g_num;` 是文件内私有，`extern` 也无法跨文件访问）。
3. **全局常量默认是内部链接**：
   `const int MAX = 100;`（全局）默认只能在当前文件访问，想跨文件共享，需要写 `extern const int MAX = 100;`（定义） + `extern const int MAX;`（声明）。

---
### 关键字virtual

`virtual`是用于实现**多态**的核心关键字，主要用在类的成员函数上。
- 普通成员函数：就像固定的指令，调用时直接执行“写死”的版本。在编译阶段就绑定地址
- 虚函数（`virtual` 修饰的函数）：就像一个“接口/占位符”，程序运行时会根据**实际对象的类型**，动态选择要执行的函数版本，而不是根据指针/引用的类型。在运行阶段再绑定地址

1. 基础定义：虚函数的声明
在基类中用 `virtual` 修饰成员函数，子类可以重写（override）这个函数：

```cpp
// 基类：动物
class Animal {
public:
    // 声明虚函数：发出声音
    virtual void makeSound() {
        cout << "动物发出声音" << endl;
    }
    // 虚析构函数（重要！）
    virtual ~Animal() {}
};
// 子类：猫
class Cat : public Animal {
public:
    // 重写基类的虚函数（override可选，但建议加，增强可读性）
    void makeSound() override {
        cout << "喵喵喵" << endl;
    }
};
// 子类：狗
class Dog : public Animal {
public:
    void makeSound() override {
        cout << "汪汪汪" << endl;
    }
};
int main() {
    // 多态核心：基类指针/引用指向子类对象
    Animal* animal1 = new Cat();
    Animal* animal2 = new Dog();
    // 运行时动态选择对应子类的函数
    animal1->makeSound(); // 输出：喵喵喵
    animal2->makeSound(); // 输出：汪汪汪
    // 释放内存
    delete animal1;
    delete animal2;
    return 0;
}
```
2. 关键细节
- **虚函数的生效条件**：
  必须通过**基类的指针或引用**调用，才能触发多态；如果直接用子类对象调用，和普通函数效果一致。
- **虚析构函数**：
  如果基类有虚函数，建议将析构函数也声明为 `virtual`。否则当用基类指针删除子类对象时，可能只调用基类析构函数，导致子类的资源泄漏。
- **override 关键字**：
  C++11 新增的 `override` 不是必须的，但加在子类重写的函数后，编译器会检查是否真的重写了基类的虚函数（比如函数名、参数写错时会报错），避免低级错误。



3. **纯虚函数（virtual xxx xxx() = 0）**：

    包含纯虚函数的类就是 “抽象类”，抽象类的核心规则是：不能直接创建对象（实例化）；如果子类继承了抽象类，但没有把父类所有的纯虚函数都重写实现，那么这个子类也会变成抽象类，同样不能实例化。
    virtual void makeSound() = 0;就是纯虚函数
    virtual void makeSound();是普通虚函数，默认提供实现
   ```cpp
   class Animal {
   public:
       // 纯虚函数
       //注意不是virtual void makeSound();
       //virtual void makeSound();是普通虚函数，默认提供实现
       //可以实例化，子类可以选择性重写
       virtual void makeSound() = 0;//纯虚函数，必须在子类中重写实现，否则无法实例化
       virtual ~Animal() {}
   };
   // Cat必须重写makeSound，否则Cat也是抽象类，无法实例化
   class Cat : public Animal {
   public:
       void makeSound() override {
           cout << "喵喵喵" << endl;
       }
   };
   ```
4. **virtual 不能修饰的内容**：
   - 全局函数、静态成员函数（static）；
   - 构造函数（构造函数执行时，对象的多态性还未建立，无法虚调用）；
   - 内联函数（inline）：加 `virtual` 后，inline 会失效（除非通过对象直接调用，而非指针/引用）

5. **虚析构函数**

- 在使用继承和多态时，基类的析构函数应声明为虚函数，以确保通过基类指针删除派生类对象时，
- 派生类的析构函数能够被正确调用，避免资源泄漏。
```cpp
class Base {
public:
    ~Base() { // 非虚析构
        cout << "Base 析构" << endl;
    }
};
class Derived : public Base {
public:
    ~Derived() {
        cout << "Derived 析构" << endl; // 不会被调用！
    }
};

int main() {
    Base* p = new Derived(); // 基类指针指向派生类对象
    delete p; // 仅调用 Base 析构，Derived 析构被跳过,内存未释放
    return 0;
}
```
---
### 关键字inline
1. **内联调用函数**
   - 压栈、跳转、返回、出栈
   这些都要消耗 CPU。
   `inline` 可以`建议`编译器：**直接把函数代码复制到调用处**，不做函数调用，适合`高频`调用的`小`函数

2. **解决头文件重复定义问题**
   在 C++ 里，如果**普通函数写在头文件**，被多个 `.cpp` 包含，会报 **multiple definition** 错误。
   加 `inline` 后，编译器允许它在多个编译单元存在，不会重复定义。


#### 函数调用的方式
- 常规调用
- 内联调用（插入到调用处代码中）

**常规函数调用**
   - 压栈、跳转、返回、出栈

当程序执行到一个普通函数调用时，CPU 需要完成一系列操作，主要包括：

1. **保存当前上下文（现场）**  
   - 将当前函数的寄存器状态（如通用寄存器、程序计数器 PC 等）压入栈中。
   - 特别是返回地址（即调用结束后应继续执行的位置）会被压栈。

2. **参数传递**  
   - 参数通常通过寄存器或栈传递（取决于调用约定，如 `cdecl`、`stdcall` 等）。

3. **跳转到目标函数地址**  
   - CPU 的程序计数器（PC）被设置为被调函数的入口地址。

4. **创建新的栈帧（Stack Frame）**  
   - 被调函数在栈上分配空间用于局部变量、保存调用者栈帧指针等。

5. **执行函数体**

6. **销毁栈帧并恢复现场**  
   - 函数返回前，清理局部变量，恢复调用者的栈帧。

7. **跳回调用点继续执行**



**内联展开的优势**
- **消除函数调用开销**：无需压栈、跳转、建栈帧、返回等操作。
- **提升指令局部性**：有利于 CPU 指令缓存（I-Cache）命中。
- **便于后续优化**：编译器可以对展开后的代码进行常量传播、死代码消除等优化。

- 代价：
- **代码膨胀**：每次调用都插入一份函数体，可能导致可执行文件变大。
- **缓存压力增加**：过大的代码体积可能降低 CPU 指令缓存效率，反而降低性能



#### inline 的关键点
1. **它只是“建议”，不是命令**
   函数太长、有循环/递归，编译器会**忽略 inline**，还是当成普通函数。

2. **适合极短的函数**
   - getter/setter
   - 简单算术、小逻辑
   不适合：复杂逻辑、循环、递归、大函数。

3. **类内定义的函数默认 inline**

4. **现代编译器很智能**
   即使你不写 `inline`，编译器在开启优化（`O2`）时，也会自动把小函数 inline。

5. **什么时候该用 inline？**
- 函数**很短**，几行代码
- 被**高频调用**（循环里、热点路径）
- 函数要放在**头文件**里被多处包含

6. **什么时候不要用？**
- 函数大、有循环、递归
- 很少调用的函数
- 虚函数（一般无法 inline,否则inline会失败）


---
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

---
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
4. **new可以通过operator重载**
```cpp
// 全局重载 operator new
void* operator new(size_t size) {
    cout << "全局 new：分配 " << size << " 字节" << endl;
    void* p = malloc(size);
    if (!p) throw bad_alloc();
    return p;
}
// 全局重载 operator delete
void operator delete(void* p) {
    cout << "全局 delete：释放内存" << endl;
    free(p);
}
int main() {
    int* arr = new int[5]; // 调用全局重载的 new
    delete[] arr; // 注意：数组需重载 operator new[]/delete[]
    return 0;
}
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
---
### 联合体union
- 在 C++ 中，**联合体（`union`）是一种特殊的复合数据类型**，其核心特点是：**所有成员共享同一块内存空间**。
- 结构体是所有成员都占有内存，联合体是所有成员共享内存
#### 性质
1. **内存共享**
- 所有成员从**同一内存地址开始**。
- 修改一个成员会**覆盖其他成员的数据**。

2. **大小由最大成员决定**
```cpp
union Data {
    int i;          // 4 bytes
    double d;       // 8 bytes
    char str[20];   // 20 bytes
};
// sizeof(Data) == 20 (假设无特殊对齐)
```
3. **同一时间只能有一个成员有效**
- 联合体不记录当前哪个成员是“活跃的”。
- 程序员必须**自己跟踪当前存储的是哪种类型**（通常配合枚举使用）。

**示例**
```cpp
#include <iostream>
using namespace std;
union Value {
    int i;
    float f;
    char c;
};

int main() {
    Value v;
    v.i = 65;        // 写入 int
    cout << v.c << endl; // 读作 char → 输出 'A'（65 的 ASCII）
    v.f = 3.14f;     // 写入 float，覆盖之前的值
    cout << v.i << endl; // 读作 int → 输出垃圾值（位模式被解释为整数）
}
```

**危险**：如果写入 `float` 却读取 `int`，结果是未定义的（取决于位表示）。

**C++11 对联合体的增强**
1. **支持非平凡类型（但需手动管理生命周期）**
C++11 之前，`union` 只能包含**平凡类型**（如 `int`, `float`, 指针等），不能包含 `std::string`、带构造函数的类等。
C++11 起，允许包含非平凡类型，但：
- **不会自动调用构造函数/析构函数**
- **程序员必须手动调用 placement new 和显式析构**

**示例：安全使用 `std::string` in union**
```cpp
#include <string>
#include <new> // for placement new

union SafeUnion {
    int i;
    std::string s;
    // 构造函数
    SafeUnion() : i(0) {}
    // 析构函数必须显式定义
    ~SafeUnion() {
        // 必须手动析构 string
        s.~basic_string();
    }
    // 安全赋值 string
    void setString(const std::string& str) {
        new(&s) std::string(str); // placement new
    }
};
int main() {
    SafeUnion u;
    u.setString("Hello");
    std::cout << u.s << std::endl;
    // 析构时自动调用 ~SafeUnion()
}
```

**关键**：一旦 union 包含非平凡类型，就必须自定义析构函数，并手动管理对象生命周期

---
### Lambda表达式
```cpp
[capture](parameters) -> return-type {
    // 函数体
}
```
[capture] 是捕获列表，用于指定Lambda表达式中可以访问的变量。
- 可以捕获外部变量，分为值捕获x和引用捕获&x。
- 值捕获时，如果在表达式之后修改x变量，则捕获的x变量不会改变。
- 引用捕获&x，如果在表达式之后修改x变量，则捕获的x变量也会改变。
- 可以&&捕获，表示捕获的是右值引用，从而捕获自己(递归)
(parameters)是参数列表，调用时传入实参。
return-type 是返回类型，可省略(自动推导)

1. **基本定义**

| 项目 | 捕获列表 `[...]` | 参数列表 `(...)` |
|------|------------------|------------------|
| **位置** | lambda 最开头，方括号内 | 捕获列表之后，圆括号内 |
| **语法示例** | `[x, &y]` | `(int a, const std::string& b)` |
| **作用** | **从定义处的外部作用域“带入”变量** | **在调用时由调用者传入值** |

2. **核心区别：何时绑定？**

| 特性 | 捕获列表 | 参数列表 |
|------|--------|--------|
| **绑定时机** | **定义 lambda 时**（创建闭包时） | **调用 lambda 时** |
| **数据来源** | 定义 lambda 所在作用域的局部变量、this 等 | 调用表达式中传入的实参 |
| **生命周期影响** | 按引用捕获时，需确保外部变量在 lambda 调用时仍有效 | 参数是调用时的副本或引用，生命周期由调用栈管理 |

```cpp
int main() {
    int base = 10;
    int multiplier = 2;
    // 捕获 base（按值），参数为 x
    auto lambda = [base](int x) {
        return base + x;  // base 来自定义时，x 来自调用时
    };
    std::cout << lambda(5) << "\n"; // 输出 15 (10 + 5)
    base = 100; // 修改外部 base
    std::cout << lambda(5) << "\n"; // 仍输出 15！因为 base 是按值捕获的副本
}
```
> **关键点**：  
> - `base` 在 lambda **定义时**被捕获（值为 10），之后外部 `base` 改变不影响 lambda 内部。  
> - `x` 在每次 **调用时**传入（如 5），可以不同。

3. **能否修改？**

| 捕获方式 | 能否在 lambda 内修改？ |
|--------|---------------------|
| `[x]`（按值） | ❌ 默认不能（C++11/14 中是 `const`）✅ 加 `mutable` 可修改副本（不影响外部） |
| `[&x]`（按引用） | ✅ 可直接修改外部 `x` |
而**参数**总是可以在函数体内修改（除非声明为 `const`）：
```cpp
auto f = [x](int y) mutable {
    x += 10;   // 修改捕获的副本（需 mutable）
    y *= 2;    // 修改参数（允许）
    return x + y;
};
```

捕获外部变量
- Lambda 可以“捕获”其定义作用域中的变量。

| 语法       | 含义 |
|-----------|------|
| `[x]`     | **按值捕获** `x`（只读副本） |
| `[&x]`    | **按引用捕获** `x`（可修改原变量） |
| `[=]`     | 按值捕获**所有**外部变量（隐式） |
| `[&]`     | 按引用捕获**所有**外部变量 |
| `[x, &y]` | 混合：`x` 按值，`y` 按引用 |
| `[this]`  | 捕获当前对象的指针（C++11） |
| `[=, this]` 或 `[&, this]` | C++17 起允许（避免歧义） |

```cpp
int main() {
    int a = 10;
    int b = 20;
    // 按值捕获 a，按引用捕获 b
    auto lambda = [a, &b]() {
        std::cout << "a = " << a << "\n";      // 10
        std::cout << "b = " << b << "\n";      // 20
        // a = 100;   // ❌ 错误！按值捕获默认是 const（C++11/14）
        b = 200;      // ✅ 可修改引用
    };
    lambda();
    std::cout << "After: b = " << b << "\n"; // 200
}
```
---
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

---
### 引用&
**引用（Reference）** 是一种特殊的变量类型，它为已存在的变量创建一个“别名”（alias）
- 底层实现是指针：`int* const ref = &a;`
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

4. **作为函数参数**  
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
5. **作为函数返回值**  
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
6. **const变量的引用**
- const变量的引用也必须是const引用，且必须初始化
```cpp
const int a = 10;
const int &r =a;
```
---
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

---
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




---
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

---
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

#### 类型
**加法运算符重载+**
```cpp
Person operator+(const Person& p1, const Person& p2) {//const T&可以绑定T&和const T&
    Person p;                                         //T&只能绑定T&，因此建议const 且更安全
    p.age = p1.age + p2.age;
    return p;//返回对象本身，触发拷贝。如果返回引用会错误，因为源对象被销毁
}
```
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
**关系运算符重载==, !=, <, >, <=, >=** 
```cpp
bool operator==(const Person& p) const/*表示这个成员函数不会修改调用它的那个对象（即 *this）的状态*/ 
{
    if (age != p.age)
    {
        return false;
    }
    return true;
}
```

**函数调用运算符重载()**
- 函数调用运算符重载允许你将一个对象像函数一样被调用。这使得类的实例可以“表现”为可调用的实体，是实现仿函数或函数对象的关键
- `operator()`是一个特殊的运算符，它没有参数列表时也必须写成`()`
- 它可以接受任意数量和类型的参数,可以是 const 修饰的，也可以不是,可以有默认参数

```cpp
// 在类内部定义
返回类型 operator()(参数列表) {//调用时关键字会被省略，只剩下(参数列表)
    // 实现函数体
}
class Person {
    private:
    int age;
    public:
    Person(int age1): age(age1) {};//构造函数
    int operator()(int x)  {//调用时关键字会被省略，只剩下(参数列表)
    return age += x;
    }
};
int main() {
        Person p10(10), p20(20);
        cout << p10(5);//相当于p10.operator()(5)，此时p10.age=15
        cout << p20(5);//相当于p20.operator()(5)，此时p20.age=25
        cout << p10(5);//不是p10()5,a + b：a 是左操作数，b 是右操作数。
                       //p10(5)：p10 是左操作数，(5) 是右操作数。
        return 0;
    }
```

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



---
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


---
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
- C语言的`struct`结构体成员不能有函数，C++的`struct`可以,且默认权限是`public`
- C++的`struct`和`class`的唯一区别是默认权限不同，很多时候`class`换成`struct`也能正常编译运行
- `struct`也能添加private等控制符控制权限
- `class`继承时未注明默认private，`struct`继承时未注明默认public
- `struct`也可以构造/析构，继承、多态：`struct Derived : Base {}`
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

- 静态/非静态成员函数存储在代码区，不存储在类中，不占用类的空间
- 静态成员变量在bss段/data段，不在类中
- 非静态成员变量存储在栈/堆，存储在类中，占用类的空间，首个非静态成员变量的地址就是类的对象地址
- 如果有虚函数，对象的地址是虚函数指针的地址，第二个地址才是数据成员变量的地址
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


---
### 继承

```cpp
class 派生类名 : 访问控制符 基类名 {
    // 派生类的成员（数据成员和成员函数）
};
```

**访问控制符**
- 不指定访问控制符，类的继承默认为 `private`
- `public`：公有
- `protected`：保护
- `private`：私有

| 排名 | 权限类型 | 严格程度 | 说明 |
|------|----------|----------|------|
| 1 | 私密 (Private) |最严格 | 只能被定义它的类内部访问。外部代码、派生类、其他类均无法直接访问。是封装性的核心体现。 |
| 2 | 保护 (Protected) |中等严格 | 可以被类自身成员和其派生类成员访问，但不能被类的对象或外部函数直接访问。主要用于继承中的共享。 |
| 3 | 公开 (Public) |最宽松 | 可以被任何代码访问，包括类外、派生类、外部函数等。是对外暴露接口的方式。 |

- Private>Protected>Public
- 因此继承时，访问控制符和基类的权限中取`最严格的权限`，私有无法访问，但是会被继承(占用空间)
- 例如基类中protected中的东西，被子类中public继承，则子类中继承的权限依旧为protected
- 基类中public中的东西，被子类中protected继承，则子类中继承的权限变为protected
- 基类中public中的东西，被子类中private继承，则子类中继承的权限变为private

**对象模型**
- 父类的非静态成员都会被继承，会占用空间，private也会，只是会被隐藏限制访问
- 父类的静态成员不会被继承，属于类本身，不会占用空间
```cpp
class Base {
    private:
    int a;
    protected:
    int b;
    public:
    int c;
};
class Derived : public Base {
    public:
    int d;
};
//sizeof(Derived) == sizeof(Base) + sizeof(int)  >>   16
```

**公有继承**

```cpp
// 基类
class Animal {
public:
    void eat() {
        cout << "This animal is eating." << endl;
    }
    void sleep() {
        cout << "This animal is sleeping." << endl;
    }
};
// 派生类：狗（Dog 继承自 Animal）
class Dog : public Animal {
public:
    void bark() {
        cout << "Woof! Woof!" << endl;
    }
};
int main() {
    Dog myDog;
    myDog.eat();     // 继承自 Animal
    myDog.sleep();   // 继承自 Animal
    myDog.bark();    // Dog 自有的方法
    return 0;
}
```
#### 继承的构造函数与析构函数
- 派生类的构造函数必须显式调用父类的构造函数，除非是无参构造，否则会报错
- 析构不用显式调用
- 构造函数会自动按顺序调用：先基类，再派生类。
- 析构函数会自动按逆序调用：先派生类，再基类。

#### **继承中的同名成员(静态/非静态)**
- 调用时默认为子类的成员
- 想调用父类的成员应该加作用域
```cpp
class Base {
    public:
    int a = 10;
    Base(int x) :a(x){}
};
class Derived : public Base {
    public:
    int a = 20;
    Derived(int x) :Base(x), a(x){}//显式调用父类的构造函数
};
int main() {
    Derived d(20);
    cout << d.a << endl; // 默认访问子类成员
    cout << d.Base::a << endl; // 添加作用域访问父类成员
}
```



#### 多重继承
- 一个类可以继承多个基类(不建议)：
- 容易出现同名成员，要带作用域
```cpp
class A {};
class B {};
class C : public A, public B {};  // 多重继承
```


**钻石问题(菱形继承)**
```cpp
      A//基类
     / \
    B   C//都是派生类
     \ /
      D //D继承了A和B
      //D通过两条路径继承了A
class A {
public:
    int data = 100;
    void print() {
        cout << "A::print()" << endl;
    }
};
class B : public A {};
class C : public A {};

class D : public B, public C {};  // 多重继承

int main() {
    D d;
    // d.data;           //编译错误！二义性！
    // d.print();        //同样二义性！
    cout << d.B::data << endl;//正确
    cout << d.C::data << endl;//正确
    return 0;
}
```      
**解决方法**
- 引入虚继承virtual
```cpp
class A {
public:
    int data = 100;
    void print() {
        cout << "A::print()" << endl;
    }
};
class B : virtual public A {};   // 虚继承
class C : virtual public A {};   // 虚继承
class D : public B, public C {}; // 此时只有一个 A 副本
int main() {
    D d;
    cout << d.data << endl;       // 正确输出：100
    d.print();                    // 正确调用
    return 0;
}
```
| 普通继承 | 虚继承 |
|---------|--------|
| 每个派生类都拥有独立的基类副本 | 所有派生类共享同一个基类实例 |
| 内存中存在多个 `A` | 内存中只有一个 `A` |
| 导致二义性 | 消除二义性 |

- 虚基类的构造函数必须由最远的派生类即D直接调用，B和C不能调用A的构造
- 且D不能调用B  C的构造
```cpp
class D : public B, public C {
public:
    D(int x) : A(x), B(x), C(x) {}  //错误！因为B和C没有B(int x)、C(int x)构造函数
    
    D(int x) : A(x) {}  // ✅ 只有D调用A，如果B或C存在有参构造函数，D就要显式调用它们
};
```
---
### 多态

**静态多态**：函数模板、函数重载


**动态多态：**
1. 有继承关系
2. 子类重写父类的虚函数(virtual),函数返回值类型 函数名 参数列表完全相同
3. 父类指针或引用指向子类对象时，调用的是子类的函数
```cpp
class Animal {//父类
    public:
    void makeSound()//普通函数
    {
        cout << "动物发出声音" << endl;
    }
    virtual void makeSound1()//虚函数
    {
        cout << "动物发出声音1" << endl;
    }

};
class Dog : public Animal { //子类
    public:
    void makeSound()//重写父类的普通函数
    {
        cout << "汪汪汪" << endl;
    }
    void makeSound1()//重写父类的虚函数
    {
        cout << "汪汪汪1" << endl;
    }

};
dospeak(Animal& animal) {//父类指针或引用调用子类，编译时确定了函数，因此调用父类的
    animal.makeSound();
}
dospeak0(Dog& animal) {//指针或引用直接调用子类，编译时确定了函数，因此调用子类的
    animal.makeSound();
}
dospeak1(Animal& animal) {//父类指针或引用调用子类，运行时才确定函数，因此调用子类的
    animal.makeSound1();}
int main() {
    Dog dog;
    dospeak(dog);//我们希望输出“汪汪汪”，实际输出“动物发出声音”
    dospeak0(dog);//成功输出“汪汪汪”
    dospeak1(dog);//成功输出“汪汪汪1”
}
```

**虚函数表与虚函数指针**
- 虚函数表：属于类，每类一份，保存所有的虚函数的函数指针，是程序的只读代码段
- 虚函数指针：属于对象，每个对象一份，指向其类对应的虚函数表，存储在对象的内存空间
- 因此一个包含虚函数的类的实例化对象，其大小至少是8字节(64位系统)，因为虚函数指针
- 所有同类对象都指向同一份虚函数表，子类重写虚函数时，会覆盖父类的虚函数表
---
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
| 移动构造函数       | 参数为“同类对象的右值引用T&&”，用于资源转移，无效拷贝 | `Student(Student&& other) { name = move(other.name); age = other.age; }` |


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

const 成员（const int age;）：const 成员必须在初始化列表初始化(或者类中)，不能在构造函数体中赋值。
- 因为函数会先统一初始化，再调用构造函数赋值，赋值了两次，所以const成员必须在初始化列表初始化。

引用成员（int& b;）：引用必须在创建时绑定对象，只能用初始化列表。
- 同上

基类的有参构造函数：基类的有参构造函数必须在初始化列表中调用。
- 有参构造如果不在初始化列表调用，就没有参数，同时又没有默认构造，就会出错

成员对象存在有参构造：必须在初始化列表中调用
- 理由同上，成员对象的类无默认构造，如果不在初始化列表显显式调用，就没有参数，同时又没有默认构造，就会出错
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

---
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

---
## 模板

### 函数模板
C++ 中的**函数模板**是一种泛型编程工具，允许你编写与类型无关的函数，编译器会根据调用时传入的实参类型，自动生成对应类型的函数实例（模板实例化）。它的核心价值是**代码复用**，避免为不同类型重复编写逻辑相同的函数（比如交换两个整数、两个字符串、两个自定义对象的逻辑完全一致）。

#### 基本语法
函数模板的定义以 `template` 关键字开头，后跟**模板参数列表**（用尖括号 `<>` 包裹），然后是普通函数的定义。

**语法格式**
```cpp
template <typename T1, typename T2>  // 模板参数列表：T是类型参数（typename 可替换为 class）
返回值类型 函数名(参数列表) {
    // 函数逻辑（使用T1, T2 作为类型）
}
```
- `template`：声明这是一个模板。
- `<typename T1, typename T2>`：模板参数列表，`T1` 是**类型形参**（可以理解为“类型占位符”），`typename` 和 `class` 在此处等价（推荐用 `typename` 更语义化）。
- 函数体内可以像使用普通类型（如 `int`、`string`）一样使用 `T1, T2`。

- 最简单的示例：交换两个值
```cpp
#include <iostream>
using namespace std;
// 函数模板：交换两个任意类型的值
template <typename T>
void swapValue(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}
int main() {
    // 实例化1：T = int
    int x = 10, y = 20;
    swapValue(x, y);
    cout << "int交换后：x=" << x << ", y=" << y << endl;  // 输出：x=20, y=10
    // 实例化2：T = double
    double m = 3.14, n = 6.28;
    swapValue(m, n);
    cout << "double交换后：m=" << m << ", n=" << n << endl;  // 输出：m=6.28, n=3.14
    // 实例化3：T = string
    string s1 = "hello", s2 = "world";
    swapValue(s1, s2);
    cout << "string交换后：s1=" << s1 << ", s2=" << s2 << endl;  // 输出：s1=world, s2=hello
    return 0;
}
```

#### 分类
函数模板的参数分为两类：**类型参数** 和 **非类型参数**。

**1. 类型参数（最常用）**
用 `typename T` 或 `class T` 声明，表示“任意类型”（内置类型、自定义类型均可）。
- 可以声明多个类型参数：
  ```cpp
  template <typename T1, typename T2>
  T1 add(T1 a, T2 b) {
      return a + b;  // 自动类型转换为 T1
  }

  // 调用
  int res1 = add(10, 3.14);  // T1=int, T2=double → 返回13
  double res2 = add(3.14, 10);  // T1=double, T2=int → 返回13.14
  ```

**非类型参数（数值参数）**
表示一个**常量值**（必须是编译期可确定的常量），语法是直接写类型（如 `int`、`size_t`），而非 `typename`。
- 常用场景：处理数组、固定长度的逻辑。
  ```cpp
  // 非类型参数 N：数组长度（编译期常量）
  template <typename T, int N>
  T sumArray(T (&arr)[N]) {  // 数组引用，避免数组退化为指针
      T sum = 0;
      for (int i = 0; i < N; ++i) {
          sum += arr[i];
      }
      return sum;
  }

  // 调用
  int arr1[] = {1,2,3,4,5};
  cout << sumArray(arr1) << endl;  // N=5，输出15

  double arr2[] = {1.1, 2.2, 3.3};
  cout << sumArray(arr2) << endl;  // N=3，输出6.6
  ```
  ⚠️ 注意：非类型参数的类型限制（C++11/17/20 逐步放宽）：
  - 只能是整数类型（`int`、`long`、`size_t`）、枚举、指针/引用（指向全局变量/函数）、`std::nullptr_t`；
  - 不能是浮点数（`double`、`float`）、类类型（如 `std::string`）。

#### 函数模板的实例化
编译器不会直接编译模板本身，而是在**调用时**根据实参类型生成具体的函数（称为“实例化”）。实例化分为两种：

1. 隐式实例化（最常用）
编译器自动推导模板参数的类型，无需手动指定：
```cpp
swapValue(x, y);  // 隐式推导 T=int
```

2. 显式实例化
手动指定模板参数类型（解决推导失败、强制特定类型的场景）：
```cpp
// 显式指定 T=double
swapValue<double>(x, y);  // 即使x/y是int，也会先转换为double再交换

// 推导失败的场景（比如参数类型不一致）
template <typename T>
T add(T a, T b) { return a + b; }

// add(10, 3.14);  // 报错：无法推导 T（int和double冲突）
add<int>(10, 3.14);    // 显式指定 T=int，3.14转为int（3），返回13
add<double>(10, 3.14); // 显式指定 T=double，10转为double，返回13.14
```

#### 函数模板的重载
函数模板可以和普通函数重载，也可以和其他模板重载。编译器会按**优先级**选择调用的函数：

**重载规则（优先级从高到低）**
1. 普通函数（非模板）；
2. 模板特化（见下文）；
3. 模板函数（匹配度更高的优先）。

**示例：模板重载**
```cpp
#include <iostream>
using namespace std;

// 1. 普通函数（处理int）
int add(int a, int b) {
    cout << "普通函数：";
    return a + b;
}

// 2. 模板函数（通用类型）
template <typename T>
T add(T a, T b) {
    cout << "模板函数：";
    return a + b;
}

// 3. 模板重载（两个不同类型）
template <typename T1, typename T2>
auto add(T1 a, T2 b) {  // C++14 起 auto 自动推导返回值
    cout << "重载模板：";
    return a + b;
}

int main() {
    cout << add(10, 20) << endl;        // 调用普通函数（优先级更高）→ 输出：普通函数：30
    cout << add(10.5, 20.5) << endl;    // 调用模板函数 → 输出：模板函数：31
    cout << add(10, 20.5) << endl;      // 调用重载模板 → 输出：重载模板：30.5
    return 0;
}
```

#### 函数模板的特化
当模板对某些特定类型需要特殊逻辑时，使用**模板特化**（覆盖模板的通用逻辑）。

#### 语法：显式特化（全特化）
```cpp
// 通用模板
template <typename T>
bool isEqual(T a, T b) {
    return a == b;
}

// 对 T=char* 的特化（处理字符串比较）
template <>
bool isEqual<char*>(char* a, char* b) {
    return strcmp(a, b) == 0;  // 通用模板的 == 会比较指针地址，特化后比较字符串内容
}

// 调用
char* s1 = "hello";
char* s2 = "hello";
char* s3 = "world";
cout << isEqual(s1, s2) << endl;  // 特化版本，输出1
cout << isEqual(s1, s3) << endl;  // 特化版本，输出0
```

#### 注意事项
1. **模板的可见性**：模板的定义（实现）必须在调用点可见（不能只声明模板，把实现放在 `.cpp` 文件）。
   - 原因：模板实例化需要看到完整的模板代码，因此模板通常定义在头文件（`.h`/`.hpp`）中。

2. **类型限制**：模板中的操作必须对实例化的类型有效。
   ```cpp
   template <typename T>
   void print(T a) {
       cout << a << endl;
   }

   struct MyStruct {};
   // print(MyStruct{});  // 报错：cout 不支持 MyStruct 的输出（没有重载<<）
   ```
   解决：为自定义类型重载运算符（如 `operator<<`），或为模板特化。

3. **与 constexpr/consteval 结合**：C++11 起，模板函数可以是 `constexpr`（编译期计算），C++20 起支持 `consteval`（强制编译期计算）：
   ```cpp
   template <int N>
   constexpr int factorial() {
       return N <= 1 ? 1 : N * factorial<N-1>();
   }

   int main() {
       constexpr int res = factorial<5>();  // 编译期计算 5! = 120
       cout << res << endl;
       return 0;
   }
   ```


### 类模板
---
## 智能指针
C++ 智能指针（Smart Pointers）是C++11引入的一组模板类，用于自动管理动态分配的内存，从而避免内存泄漏、悬空指针等问题。它们通过RAII机制，在对象生命周期结束时自动释放所管理的资源。（定义在 `<memory>` 头文件中）：

### 1. `std::unique_ptr`
- **独占所有权**：一个资源只能被一个 `unique_ptr` 拥有。
- **不可复制**，但可以**移动**
- 析构时自动调用 `delete`


```cpp
#include <memory>
#include <iostream>

int main() {
    std::unique_ptr<int> ptr(new int(42));
    // 或使用 make_unique（推荐）
    auto ptr2 = std::make_unique<int>(42);

    std::cout << *ptr << std::endl;// 输出 42

    // 转移所有权
    auto ptr3 = std::move(ptr); // ptr 现在为 nullptr
}
```
> ✅ 推荐优先使用 `std::make_unique<T>(args...)` 创建 `unique_ptr`。


### 2. `std::shared_ptr`
- **共享所有权**：多个 `shared_ptr` 可以指向同一对象。
- 使用**引用计数**机制：当最后一个 `shared_ptr` 被销毁时，资源才被释放。
- 线程安全（引用计数操作是原子的），但所指对象的访问不是线程安全的。
- 有一定性能开销（引用计数 + 控制块）。

```cpp
#include <memory>
#include <iostream>

int main() {
    auto sp1 = std::make_shared<int>(100);
    auto sp2 = sp1; // 引用计数变为 2

    std::cout << *sp1 << ", use_count: " << sp1.use_count() << std::endl;

    // 当 sp1 和 sp2 都离开作用域，资源被释放
}
```

> ✅ 推荐使用 `std::make_shared<T>(args...)` 创建 `shared_ptr`（更高效，一次内存分配）。

---

### 3. `std::weak_ptr`
- **不拥有资源**，是对 `shared_ptr` 所管理对象的**弱引用**。
- 用于**打破循环引用**（circular reference）问题。
- 不能直接解引用，必须先通过 `lock()` 转换为 `shared_ptr`。

```cpp
#include <memory>
#include <iostream>

int main() {
    std::shared_ptr<int> sp = std::make_shared<int>(200);
    std::weak_ptr<int> wp = sp;

    if (auto locked = wp.lock()) {
        std::cout << *locked << std::endl; // 安全访问
    } else {
        std::cout << "Object already destroyed." << std::endl;
    }
}
```

> ⚠️ 循环引用示例（两个 `shared_ptr` 互相持有对方）会导致内存泄漏，此时应将其中一个改为 `weak_ptr`。
---
## 哈希结构
- 数组
- set集合
- map映射

| 集合               | 底层实现 | 是否有序 | 数值是否可以重复 | 能否更改数值 | 查询效率 | 增删效率 |
|--------------------|----------|----------|------------------|--------------|----------|----------|
| std::set           | 红黑树   | 有序     | 否               | 否           | O(log n) | O(log n) |
| std::multiset      | 红黑树   | 有序     | 是               | 否           | O(log n) | O(log n) |
| std::unordered_set | 哈希表   | 无序     | 否               | 否           | O(1)     | O(1)     |

| 映射               | 底层实现 | 是否有序   | 数值是否可以重复 | 能否更改数值 | 查询效率 | 增删效率 |
|--------------------|----------|------------|------------------|--------------|----------|----------|
| std::map           | 红黑树   | key有序    | key不可重复      | key不可修改  | O(log n) | O(log n) |
| std::multimap      | 红黑树   | key有序    | key可重复        | key不可修改  | O(log n) | O(log n) |
| std::unordered_map | 哈希表   | key无序    | key不可重复      | key不可修改  | O(1)     | O(1)     |

### set集合
#### std::set
1. 定义
- 存储**唯一元素**
- 元素**自动排序**（默认升序）
- 不允许重复
- 不能直接修改元素，默认先删除再插入（会破坏有序结构）


3. 常用操作
```cpp
#include <set>
using namespace std;
//定义
set<int> s;

// 初始化方式
//对于字符串的排序，默认按字典序排序，即首个不同的字符决定大小关系
set<std::string> names = {"Alice", "Bob", "Charlie"};

//1.插入 insert
//插入后自动排序：`2,5`
s.insert(5);
s.insert(2);
s.insert(5);   // 重复无效

//2.遍历
// for循环输出所有值
//结果：Alice Bob Charlie
for (auto x : names) {
    cout << x << " ";
}
// 迭代器遍历：set<int>::iterator
for (set<int>::iterator it = s.begin(); it != s.end(); ++it) {
    cout << *it << " ";
}

//3.查找 find
auto it = s.find(2);
if (it != s.end()) {
    cout << "找到：" << *it << endl;
}

//4. 判断存在 count
// count(x) 返回等于 x 的元素个数
//由于set中元素唯一，所以count只能是0（不存在）或1（存在）
if (s.count(3)) {
    // 存在
}

//5. 删除 erase
s.erase(2);        // 按值删除，不存在也不报错
s.erase(s.find(5));// 按迭代器删除，必须保证元素存在，否则迭代器会失效导致报错
s.erase(s.begin(), s.end()); // 迭代器区间删除，等价于clear()
s.erase(s.find(5), s.end()); // 删除>=5的元素

//6.大小与清空 size, empty, clear
s.size();// 元素个数
s.empty();// 是否为空
s.clear();// 清空集合

//7.lower_bound / upper_bound（有序集合特有）
// >= 3 的第一个元素
auto it = s.lower_bound(3);
s.erase(it,s.end()) // 删除 >= 3 的元素;
// > 3 的第一个元素
auto it = s.upper_bound(3);
s.erase(it,s.end()) // 删除 > 3 的元素;
```
4. 特点总结
- 元素**唯一、有序**
- 插入/删除/查找 O(log n)
- 不能修改元素，只能删了重插
- 适合：去重、排序、快速判存在

#### std::multiset
- key **可以重复**，和set的区别就是允许重复元素
- 仍然有序
```cpp
#include <multiset>
std::multiset<int> ms = {3, 1, 4, 1, 5};
// 插入重复值有效
ms.insert(1); // 现在有三个 1

// multiset 和 set 唯一区别：允许元素重复，其他用法基本一致

// 查找所有等于 1 的元素
// equal_range(1) 返回一对迭代器，表示值为 1 的区间 [起始, 结束)
// range.first  → 第一个等于 1 的元素迭代器
// range.second → 第一个**大于 1** 的元素迭代器
// 例子：multiset 里是 {1,1,1,2,3}
// equal_range(1) 得到区间：从第1个1 → 到2的位置（不包含2）
auto range = ms.equal_range(1);
// 遍历这个区间，输出所有等于 1 的元素
for (auto it = range.first; it != range.second; ++it) {
    std::cout << *it << " ";   // 输出：1 1 1
}

// count 可大于 1
std::cout << "Count of 1: " << ms.count(1); // 3

// erase(1) 会删除所有 1！
// 若只想删一个：
auto it = ms.find(1);
if (it != ms.end()) ms.erase(it);
```

#### std::unordered_set



### map映射
#### std::map
1. 定义
- 存储 **key-value 键值对**
- **key 唯一、不可重复**
- 按 **key 自动排序**
- 通过key快速找value

map 内部每个元素是：
```cpp
pair<const Key, T>
```
- `pair.first` → key
- `pair.second` → value
因此可以用迭代器访问
```cpp
#include <map>
// 定义,map<key类型, value类型> 变量名;
map<string, int> age; // key=string，value=int

//插入，两种方式[] / insert
//key 已存在时，`[]` 会覆盖value
// insert不会覆盖，会忽略
age["张三"] = 18;
age.insert({"李四", 20});
age.insert(make_pair("王五", 22));

//访问
//若key不存在，[]会自动插入一个默认value如0
//不想插入就用 find。
cout << age["张三"];

//查找key是否存在，返回迭代器
auto it = age.find("张三");
if (it != age.end()) {
    cout << it->first << " " << it->second << endl;
}

//遍历
//本质上是迭代器遍历
for (auto& p : age) {
    cout << p.first << ": " << p.second << endl;
}

//删除
age.erase("张三");       // 按key删
age.erase(age.begin());  // 按迭代器删
age.clear();

//长度和是否为空
age.size();
age.empty();
age.count(key); // 0或1
```
5. 特点总结
- key 唯一、有序
- 查找/插入/删除 O(log n)
- key 不能修改，value 可以改
- 适合：字典、映射、索引表


#### multimap
#### unordered_map
---
## 容器
```
stack 栈（后进先出）
queue 队列（先入先出）
vector 向量（动态数组）
array 数组（固定大小）

```
### stack 栈（后进先出）

C++标准库的`std::stack`容器,底层默认是`deque`，也可`stack<int, vector<int>> s2;`显式指定为`vector`或`list`
- `stack`：C++ STL中的栈容器，遵循**后进先出（LIFO）** 规则；
- `<T>`：栈中存储的元素类型是`T`；
#### 关键用法
- **前置条件**：使用`std::stack`必须包含头文件`#include <stack>`
- **核心操作**：
  - `stk.push(t)`：将t压入栈中；
  - `stk.top()`：获取栈顶元素的引用（不弹出），需确保栈非空；
  - `stk.pop()`：弹出栈顶元素（无返回值），需确保栈非空；
  - `stk.empty()`：判断栈是否为空，为空返回true，不为空返回false；
  - `stk.size()`：获取栈中元素个数。
- **常见场景**：栈是二叉树**非递归遍历**（前序、中序、后序）的核心工具，替代递归的调用栈。

前序遍历：先访问「根节点」→ 再访问「左子树」→ 最后访问「右子树」`（根在前）`
中序遍历：先访问「左子树」→ 再访问「根节点」→ 最后访问「右子树」`（根在中）`
后序遍历：先访问「左子树」→ 再访问「右子树」→ 最后访问「根节点」`（根在后）`




### queue 队列（先入先出）
C++标准库的`std::queue`容器,底层默认是`deque`，也可`queue<int, list<int>> q2;`显式指定为`list`或`vector`
- `queue`：C++ STL中的队列容器，遵循**先进先出（FIFO）** 规则；
- `<T>`：队列中存储的元素类型是`T`；
#### 关键用法
- **前置条件**：使用`std::queue`必须包含头文件`#include <queue>`
- 只能在队首删除元素，只能在队尾插入元素
- 不能随机访问中间元素，只能访问 front() 和 back() 两个端点
- **核心操作**：
  - `q.push(t)`：将t压入队列中；
  - `q.pop()`：弹出队头元素（无返回值），需确保队列非空；
  - `q.front()`：获取队头元素的引用（不弹出），需确保队列非空；
  - `q.back()`：获取队尾元素的引用（不弹出），需确保队列非空；
  - `q.empty()`：判断队列是否为空，为空返回true，不为空返回false；
  - `q.size()`：获取队列中元素个数。
  - `q.pop()`：不返回被移除的元素！若要获取并移除，需先调用 front() 再 pop()。
