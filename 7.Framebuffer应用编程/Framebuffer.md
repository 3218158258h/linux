在 Linux 系统中，可通过 Framebuffer 驱动程序控制 LCD。其中，“Frame” 意为帧，“buffer” 意为缓冲，这意味着 Framebuffer 是一块内存区域，用于保存一帧图像的全部像素颜色值。
假设 LCD 的分辨率为 1024x768，且每个像素的颜色用 32 位表示，那么 Framebuffer 的大小可计算为：1024×768×32÷8 = 3145728 字节。

### 一、Framebuffer 基本原理
- **核心概念**：Framebuffer 被抽象为一个字符设备文件（通常是 `/dev/fb0`），应用程序通过读写该设备文件直接操作显示内存。
- **显示缓冲区**：内存中一块连续的区域，存储当前屏幕的像素数据（每个像素的颜色由 RGB 或其他格式表示）。
- **设备信息**：包含屏幕分辨率（宽×高）、像素格式（如 16bpp、24bpp、32bpp）、屏幕尺寸等，通过 `ioctl` 接口获取。



### 简单介绍 LCD 的操作原理：
1. **驱动程序设置 LCD 控制器**：
   - 根据 LCD 的参数，配置 LCD 控制器的时序和信号极性；
   - 依据 LCD 的分辨率和 BPP（每像素位数），分配 Framebuffer 内存。

2. **应用程序（APP）操作**：
   - 通过 `ioctl` 系统调用获取 LCD 的分辨率和 BPP 信息；
   - 使用 `mmap` 将 Framebuffer 映射到用户空间，直接在映射的内存中写入图像数据。


若要设置 LCD 中坐标 (x,y) 处像素的颜色，需先确定该像素在 Framebuffer 中对应的内存地址，再根据 BPP 值设置颜色。

假设 `fb_base` 是 APP 执行 `mmap` 后得到的 Framebuffer 起始地址


(x,y) 坐标处像素的起始地址可通过以下公式计算：  
`(x,y) 像素起始地址 = fb_base + (xres × bpp/8) × y + x × bpp/8`//xres是屏幕宽的像素数

### 像素颜色的表示方式
- **32BPP**：通常仅使用低 24 位表示 RGB 颜色，高 8 位表示透明度（一般 LCD 不支持透明度）。
- **24BPP**：硬件为方便处理，在 Framebuffer 中实际用 32 位存储，效果与 32BPP 一致。
- **16BPP**：常用格式为 RGB565（红 5 位、绿 6 位、蓝 5 位）；



### 二、编程步骤
#### 1. 打开 Framebuffer 设备
使用 `open` 函数打开 `/dev/fb0`（或其他帧缓冲设备），获取文件描述符：
```c
#include <fcntl.h>
#include <unistd.h>

int fb_fd = open("/dev/fb0", O_RDWR);  // 读写方式打开

```


#### 2. 获取显示设备信息
通过 `ioctl` 调用 `FBIOGET_VSCREENINFO` 获取可变屏幕信息（分辨率、像素格式等）：
```c
#include <linux/fb.h>
#include <sys/ioctl.h>

struct fb_var_screeninfo var_info;  // 可变屏幕信息


// 关键参数解析
int width = var_info.xres;        // 屏幕宽度（像素）
int height = var_info.yres;       // 屏幕高度（像素）
int bpp = var_info.bits_per_pixel; // 每像素位数（如 32）
int line_length = var_info.xres_virtual * (bpp / 8);  // 每行字节数



```

- `bits_per_pixel`（bpp）：决定像素格式，常见值：
  - 16bpp：RGB565（5 位红、6 位绿、5 位蓝）。
  - 24bpp：RGB888（每个颜色 8 位，共 24 位）。
  - 32bpp：RGBA8888（含 8 位透明度）或 XRGB8888（高 8 位未使用）。


#### 3. 映射显示缓冲区到用户空间
通过 `mmap` 将内核中的帧缓冲内存映射到用户空间，直接读写像素数据：
```c
#include <sys/mman.h>

// 计算缓冲区总大小（字节）
long int fb_size = width * height * (bpp / 8);

// 映射内存
unsigned char *fb_buf = (unsigned char *)mmap(
    NULL,           // 自动分配地址
    fb_size,        // 映射大小
    PROT_READ | PROT_WRITE,  // 可读可写
    MAP_SHARED,     // 共享映射（修改对内核可见）
    fb_fd,          // 帧缓冲文件描述符
    0               // 偏移量（从开头映射）
);

if (fb_buf == MAP_FAILED) {
    perror("mmap failed");
    close(fb_fd);
    return -1;
}
```


#### 4. 操作像素（绘制图形）
根据像素格式计算每个像素的位置和颜色值，直接修改映射的内存 `fb_buf`。

##### 示例 1：填充全屏为红色（32bpp 为例）
32bpp 中，每个像素占 4 字节（假设格式为 0xRRGGBBAA 或 0xXXRRGGBB，需根据实际设备调整）：
```c
// 红色的 32 位值（假设格式为 R=0xFF, G=0x00, B=0x00, A=0xFF）
unsigned int red = 0xFFFF0000;  // 具体值需根据设备像素格式调整

for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        // 计算当前像素在缓冲区中的地址
        //帧缓存起始地址+y轴偏移量+x轴偏移量
        unsigned int *pixel = (unsigned int *)(fb_buf + y * line_length + x * (bpp / 8));
        *pixel = red;  // 解引用写入颜色值
    }
}
```

##### 示例 2：绘制一个点（16bpp RGB565 格式）
RGB565 中，每个像素占 2 字节：红（5 位）、绿（6 位）、蓝（5 位）：
```c
// 将 RGB888 转换为 RGB565
unsigned short rgb888_to_565(unsigned char r, unsigned char g, unsigned char b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// 在 (x=100, y=200) 处绘制一个绿色点
int x = 100, y = 200;
unsigned short green = rgb888_to_565(0, 255, 0);  // 绿色
unsigned short *pixel = (unsigned short *)(fb_buf + y * line_length + x * 2);
*pixel = green;
```


#### 5. 清理资源
操作完成后，解除内存映射并关闭文件描述符：
```c
munmap(fb_buf, fb_size);  // 解除映射
close(fb_fd);             // 关闭设备
```




### 完整示例：绘制彩色渐变
```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

int main() {
    // 1. 打开帧缓冲设备
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) { perror("open"); return -1; }

    // 2. 获取屏幕信息
    struct fb_var_screeninfo var;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) == -1) { perror("ioctl"); close(fb_fd); return -1; }
    int width = var.xres, height = var.yres;
    int bpp = var.bits_per_pixel;
    long fb_size = width * height * (bpp / 8);

    // 3. 映射缓冲区
    unsigned char *fb_buf = mmap(NULL, fb_size, PROT_RW, MAP_SHARED, fb_fd, 0);
    if (fb_buf == MAP_FAILED) { perror("mmap"); close(fb_fd); return -1; }

    // 4. 绘制红蓝渐变（32bpp 示例）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 红色分量随 x 变化，蓝色分量随 y 变化
            unsigned char r = (x * 255) / width;
            unsigned char b = (y * 255) / height;
            unsigned int color = (r << 16) | (b << 0) | 0x000000FF;  // 假设格式为 0xRRGGBB

            unsigned int *pixel = (unsigned int*)(fb_buf + y * var.xres_virtual*(bpp/8) + x*(bpp/8));
            *pixel = color;
        }
    }

    // 等待用户输入后清理
    getchar();
    munmap(fb_buf, fb_size);
    close(fb_fd);
    return 0;
}
```
