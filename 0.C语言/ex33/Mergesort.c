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

// 合并两个有序链表
Node *merge(Node *l1, Node *l2) {
    Node dummy;
    Node *tail = &dummy;
    dummy.next = NULL;

    while (l1 && l2) {
        if (l1->data < l2->data) {
            tail->next = l1;
            l1 = l1->next;
        } else {
            tail->next = l2;
            l2 = l2->next;
        }
        tail = tail->next;
    }

    if (l1) {
        tail->next = l1;
    } else {
        tail->next = l2;
    }

    return dummy.next;
}

// 找到链表的中间节点
Node *getMiddle(Node *head) {
    if (!head) return NULL;
    Node *slow = head;
    Node *fast = head->next;

    while (fast && fast->next) {//fast 指针每次移动两步，slow 指针每次移动一步
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

// 归并排序
Node *mergesort(Node *head) {
    if (!head || !head->next) return head;

    Node *mid = getMiddle(head);
    Node *right = mid->next;
    mid->next = NULL;

    Node *leftSorted = mergesort(head);
    Node *rightSorted = mergesort(right);

    return merge(leftSorted, rightSorted);
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

    // 进行归并排序
    head = mergesort(head);

    printf("排序后的链表: ");
    printnode(head);

    // 释放链表内存
    freenode(head);
    return 0;
}    
