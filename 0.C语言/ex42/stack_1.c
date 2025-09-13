#include <stdio.h>
#include <stdlib.h>
#define Max 100//栈最大容量

// 定义栈结构体
typedef struct {
    int data[Max];
    int top;
} Stack;

// 初始化栈
Stack* createStack() {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    if (stack == NULL) {
        printf("内存分配失败\n");
        return NULL;
    }
    stack->top = -1;
    return stack;
}

// 压入
void push(Stack* stack, int data) {
    if (stack->top == Max - 1) {
        printf("栈已满，无法压入\n");
        return;
    }
    stack->data[++(stack->top)] = data;
}

// 压出
int pop(Stack* stack) {
    if (stack->top == -1) {
        printf("栈为空，无法压出\n");
        return -1;
    }
    return stack->data[(stack->top)--];
}

// 清空
void clean(Stack* stack) {
    free(stack);
}

// 打印
void printStack(Stack* stack) {
    if (stack->top == -1) {
        printf("栈为空\n");
        return;
    }
    for (int i = stack->top; i >= 0; i--) {
        printf("%d ", stack->data[i]);
    }
    printf("\n");
}

int main() {
    Stack* stack = createStack();
    if (stack == NULL) {
        return 1;
    }
    int num1, num2;
    printf("请输入要压入的数字个数：\n");
    scanf("%d", &num1);
    printf("请输入数字（回车结束）：\n");
    for (int i = 0; i < num1; i++) {
        int data;
        scanf("%d", &data);
        push(stack, data);
    }
    printf("已压入的数字有：");
    printStack(stack);
    printf("请输入要压出的数字个数：");
    scanf("%d", &num2);
    for (int i = 0; i < num2; i++) {
        int popped = pop(stack);
        if (popped != -1) {
            printf("弹出的数字是：%d\n", popped);
        }
    }
    printf("剩余的数字有：");
    printStack(stack);
    clean(stack);
    printf("已清空所有数字\n");
    return 0;
}    
