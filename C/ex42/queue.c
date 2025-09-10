#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// 定义队列节点
typedef struct queuenode {
    int data;
    struct queuenode *next;
} queuenode;

// 定义队列结构
typedef struct queue {
    queuenode *front;
    queuenode *rear;
} queue;

// 创建新节点
queuenode *create_node(int x) {
    queuenode *new_node = (queuenode *)malloc(sizeof(queuenode));
    if (new_node == NULL) {
        printf("内存分配失败\n");
        return NULL;
    }
    new_node->data = x;
    new_node->next = NULL;
    return new_node;
}

// 创建队列
queue *create_queue() {
    queue *q = (queue *)malloc(sizeof(queue));
    if (q == NULL) {
        printf("内存分配失败\n");
        return NULL;
    }
    q->front = NULL;
    q->rear = NULL;
    return q;
}

// 入队
void enqueue(queue *q, int data) {
    queuenode *new_node = create_node(data);
    if (q->rear == NULL) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
}

// 出队
int dequeue(queue *q) {
    if (q->front == NULL) {
        printf("队列为空\n");
        return -1;
    }
    queuenode *temp = q->front;
    int data = temp->data;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return data;
}

// 清空队列
void clean_queue(queue *q) {
    if (q->front == NULL) {
        printf("队列为空\n");
        return;
    }
    while (q->front != NULL) {
        queuenode *temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
    q->rear = NULL;
}

// 打印队列
void print_queue(queue *q) {
    if (q->front == NULL) {
        printf("队列为空\n");
        return;
    }
    queuenode *p = q->front;
    while (p != NULL) {
        printf("%d ", p->data);
        p = p->next;
    }
    printf("\n");
}

int main() {
    queue *q = create_queue();
    int num1, num2;
    printf("请输入要入队的数字个数：\n");
    scanf("%d", &num1);
    printf("请输入数字（回车结束）：\n");
    for (int i = 1; i <= num1; i++) {
        int data;
        scanf("%d", &data);
        enqueue(q, data);
    }
    printf("已入队的数字有：");
    print_queue(q);
    printf("请输入要出队的数字个数：");
    scanf("%d", &num2);
    for (int i = 1; i <= num2; i++) {
        int data = dequeue(q);
        if (data != -1) {
            printf("出队的数字是：%d\n", data);
        }
    }
    printf("剩余的数字有：");
    print_queue(q);
    clean_queue(q);
    printf("已清空所有数字\n");
    free(q);
    return 0;
}    
