#include <stdio.h>
#include <stdlib.h>

typedef struct node {  // 定义节点
    int data;
    struct node *next;
} Node;

Node *create(int data) {  // 创建节点
    Node *newnode = (Node *)malloc(sizeof(Node));
    newnode->data = data;
    newnode->next = NULL;
    return newnode;
}

void swap(Node *a, Node *b) {  // 交换数据
    int t = a->data;
    a->data = b->data;
    b->data = t;
}

void insert(Node **head, int data) {  // 在末尾插入数据
    Node *newnode = create(data);
    if (*head == NULL) {
        *head = newnode;  // 使用双指针确保能将新节点作为头节点(如果没有头节点)
    } else {
        Node *p = *head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = newnode;
    }
}

void bubblesort(Node *head) {  // 冒泡排序
    if (head == NULL) return;
    int swapped;
    Node *ptr1;
    Node *lptr = NULL;

    do {
        swapped = 0;
        ptr1 = head;

        while (ptr1->next != lptr) {
            if (ptr1->data > ptr1->next->data) {
                swap(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

void printnode(Node *head) {  // 打印
    while (head != NULL) {
        printf("%d ", head->data);
        head = head->next;
    }
    printf("\n");
}
void freenode(Node *head) {  // 释放内存
    Node *p;
    while (head != NULL) {
        p = head;
        head = head->next;
        free(p);
    }
}

int main() {
    Node *head = NULL;
    // 插入一些数据
    insert(&head, 64);
    insert(&head, 34);
    insert(&head, 25);
    insert(&head, 12);
    insert(&head, 22);
    insert(&head, 11);
    insert(&head, 90);

    printf("排序前的链表: ");
    printnode(head);

    // 进行冒泡排序
    bubblesort(head);

    printf("排序后的链表: ");
    printnode(head);

    // 释放链表内存
    freenode(head);
    return 0;
}    
