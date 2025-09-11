#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义环形缓冲区结构体
typedef struct {
    char *buf;  // 指向缓冲区的指针，用于存储数据
    int size;   // 缓冲区的大小，包含一个额外的位置用于区分缓冲区满和空的状态
    int head;   // 缓冲区的头指针，指示下一个读取数据的位置
    int tail;   // 缓冲区的尾指针，指示下一个写入数据的位置
} RingBuf;

// 创建环形缓冲区
// len 是用户指定的有效数据长度
RingBuf *create(int len) {
    // 为 RingBuf 结构体分配内存
    RingBuf *rb = malloc(sizeof(RingBuf));
    // 如果内存分配失败，返回 NULL
    if (!rb) return NULL;
    // 缓冲区的实际大小为用户指定长度加 1，用于区分满和空的状态
    rb->size = len + 1;
    // 初始化头指针和尾指针为 0，表示缓冲区为空
    rb->head = rb->tail = 0;
    // 为缓冲区的数据部分分配内存
    rb->buf = malloc(rb->size*sizeof(char));
    // 如果内存分配失败，释放之前为 RingBuf 结构体分配的内存，并返回 NULL
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    // 成功创建环形缓冲区，返回指向该缓冲区的指针
    return rb;
}

// 销毁环形缓冲区
// 释放为环形缓冲区分配的内存，防止内存泄漏
void destroy(RingBuf *rb) {
    // 如果环形缓冲区指针不为 NULL
    if (rb) {
        // 释放缓冲区的数据部分的内存
        free(rb->buf);
        // 释放 RingBuf 结构体的内存
        free(rb);
    }
}

// 计算可用数据长度
// 即当前缓冲区中已经存储的数据的字节数
int availData(RingBuf *rb) {
    // 通过尾指针减去头指针并加上缓冲区大小，再对缓冲区大小取模得到可用数据长度
    return (rb->tail - rb->head + rb->size) % rb->size;
}

// 计算可用空间长度
// 即当前缓冲区中还可以写入的数据的字节数
int space(RingBuf *rb) {
    // 缓冲区的总可用空间为缓冲区大小减 1 减去可用数据长度
    return rb->size - 1 - availData(rb);
}

// 写入数据
// 将 data 中的数据写入环形缓冲区，最多写入 len 个字节
int write(RingBuf *rb, char *data, int len) {
    // 计算当前缓冲区的可用空间
    int spaceAvailable = space(rb);
    // 实际写入的字节数为请求写入的字节数和可用空间的较小值
    int writeLen = (len < spaceAvailable)? len : spaceAvailable;
    // 循环将数据写入缓冲区
    for (int i = 0; i < writeLen; i++) {
        // 将数据写入尾指针指向的位置
        rb->buf[rb->tail] = data[i];
        // 更新尾指针，使用取模运算实现循环写入
        rb->tail = (rb->tail + 1) % rb->size;
    }
    // 返回实际写入的字节数
    return writeLen;
}

// 读取数据
// 从环形缓冲区中读取数据到 target 中，最多读取 amount 个字节
int read(RingBuf *rb, char *target, int amount) {
    // 计算当前缓冲区的可用数据长度
    int dataLen = availData(rb);
    // 实际读取的字节数为请求读取的字节数和可用数据长度的较小值
    int readLen = (amount < dataLen)? amount : dataLen;
    // 循环从缓冲区中读取数据
    for (int i = 0; i < readLen; i++) {
        // 从缓冲区头指针指向的位置读取数据到目标数组
        target[i] = rb->buf[rb->head];
        // 更新头指针，使用取模运算实现循环读取
        rb->head = (rb->head + 1) % rb->size;
    }
    // 返回实际读取的字节数
    return readLen;
}

int main() {
    // 创建一个大小为 10 的环形缓冲区
    RingBuf *rb = create(10);
    // 要写入缓冲区的数据
    char A[] = "Hello, World!";
    // 用于存储从缓冲区读取的数据的数组
    char B[20];

    // 调用 write 函数将数据写入缓冲区，并获取实际写入的字节数
    int written = write(rb, A, strlen(A));
    // 打印实际写入的字节数
    printf("写入了 %d 个字符\n", written);

    // 调用 read 函数从缓冲区读取数据，并获取实际读取的字节数
    int readNum = read(rb, B, written);
    // 在读取的数据末尾添加字符串结束符
    B[readNum] = '\0';
    // 打印实际读取的字节数和读取的数据
    printf("读取了 %d 个字符: %s\n", readNum, B);

    // 销毁环形缓冲区，释放内存
    destroy(rb);
    return 0;
}
