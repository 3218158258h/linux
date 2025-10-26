
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


### 关键字static
- 定义在函数内部则作用域在内部，每次调用不会清零
- 作用域在cpp文件则作用域限定在本文件
- 定义在类内则作用域为类，必须在类外初始化，每个对象对于同一个staic共享数据，不同static数据不共享
- 所有对象的同一静态成员变量共享数据，不同静态成员变量不共享数据
- 类中定义的静态成员变量在编译阶段分配内存;必须类外初始化，私有数据不影响初始化，但是影响读写
- 所有对象的共享同一静态成员函数
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



## 类和对象

- 三大特性：`封装` `多态` `继承`

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




### 构造函数与析构函数
**构造函数**和**析构函数**是类的两个特殊成员函数，分别负责**对象的初始化**和**对象销毁时的资源清理**，二者均由编译器自动调用，`无需手动触发`

#### 构造函数
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
- 若用户定义了拷贝构造函数，则C++不会提供无参构造和有参构造函数

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
    // 初始化列表：成员变量按“类中声明顺序”初始化，而非列表顺序
    Student(string n, int a, float s) : name(n), age(a), score(s) {
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


#### 二、析构函数（Destructor）
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



#### 四、关键注意事项
1. **默认构造函数的“消失”**：若显式定义了“有参构造函数”，编译器会**不再生成默认构造函数**，此时若需无参创建对象，需手动定义默认构造函数（如 `Student() = default;`，C++11 特性）。
2. **拷贝构造的浅拷贝问题**：编译器自动生成的拷贝构造函数是“浅拷贝”（仅复制成员变量值），若成员包含堆内存，会导致“double free”（两个对象析构时重复释放同一块内存），需显式定义“深拷贝”拷贝构造函数。
3. **析构函数与继承**：若类作为基类且可能被继承，析构函数需定义为**虚析构函数**（`virtual ~Base()`），否则派生类对象销毁时，仅会调用基类析构，导致派生类的资源泄漏。



