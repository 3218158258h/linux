#include <stdio.h>
#include <stdlib.h>

// 定义双向链表节点结构体
typedef struct ListNode {
    int value;            // 节点存储的值
    struct ListNode *next; // 指向下一个节点的指针
    struct ListNode *prev; // 指向上一个节点的指针
} ListNode;

// 定义双向链表结构体
typedef struct List {
    ListNode *first; // 指向链表第一个节点的指针
    ListNode *last;  // 指向链表最后一个节点的指针
} List;

// 创建一个新的双向链表
List* createList() {
    // 为链表结构体分配内存
    List *list = (List*)malloc(sizeof(List));
    if (list == NULL) {
        // 如果内存分配失败，返回NULL
        return NULL;
    }
    // 初始化链表的第一个和最后一个节点指针为NULL
    list->first = NULL;
    list->last = NULL;
    return list;
}

// 在链表尾部插入一个新节点
void insertAtEnd(List *list, int value) {
    // 为新节点分配内存
    ListNode *newNode = (ListNode*)malloc(sizeof(ListNode));
    if (newNode == NULL) {
        // 如果内存分配失败，直接返回
        return;
    }
    // 设置新节点的值
    newNode->value = value;
    // 新节点的下一个节点初始化为NULL，因为它是尾节点
    newNode->next = NULL;
    // 新节点的上一个节点是当前链表的尾节点
    newNode->prev = list->last;

    if (list->last == NULL) {
        // 如果链表为空，新节点既是第一个节点也是最后一个节点
        list->first = newNode;
        list->last = newNode;
    } else {
        // 如果链表不为空，将当前尾节点的下一个节点设置为新节点
        list->last->next = newNode;
        // 更新链表的尾节点为新节点
        list->last = newNode;
    }
}

// 删除指定值的节点
void deleteNode(List *list, int value) {
    // 从链表的第一个节点开始查找
    ListNode *current = list->first;
    while (current != NULL) {
        if (current->value == value) {
            if (current == list->first && current == list->last) {
                // 如果要删除的节点既是第一个节点也是最后一个节点，将链表的首尾节点指针都设为NULL
                list->first = NULL;
                list->last = NULL;
            } else if (current == list->first) {
                // 如果要删除的节点是第一个节点，将第一个节点指针指向下一个节点
                list->first = current->next;
                if (list->first!= NULL) {
                    // 如果新的第一个节点不为空，将其prev指针设为NULL
                    list->first->prev = NULL;
                }
            } else if (current == list->last) {
                // 如果要删除的节点是最后一个节点，将最后一个节点指针指向上一个节点
                list->last = current->prev;
                if (list->last!= NULL) {
                    // 如果新的最后一个节点不为空，将其next指针设为NULL
                    list->last->next = NULL;
                }
            } else {
                // 如果要删除的节点在中间，调整前后节点的指针关系
                current->prev->next = current->next;
                current->next->prev = current->prev;
            }
            // 释放要删除节点的内存
            free(current);
            // 删除节点后直接返回
            return;
        }
        // 继续查找下一个节点
        current = current->next;
    }
}

// 正向遍历链表
void traverseForward(List *list) {
    // 从链表的第一个节点开始遍历
    ListNode *current = list->first;
    while (current!= NULL) {
        // 输出当前节点的值
        printf("%d ", current->value);
        // 移动到下一个节点
        current = current->next;
    }
    // 换行，使输出格式更清晰
    printf("\n");
}

// 释放整个链表的内存
void freeList(List *list) {
    // 从链表的第一个节点开始
    ListNode *current = list->first;
    ListNode *nextNode;
    while (current!= NULL) {
        // 保存当前节点的下一个节点指针
        nextNode = current->next;
        // 释放当前节点的内存
        free(current);
        // 将当前节点移动到下一个节点
        current = nextNode;
    }
    // 释放链表结构体的内存
    free(list);
}

// 复制链表
List* copyList(List *originalList) {
    // 创建一个新的空链表
    List *newList = createList();
    if (newList == NULL) {
        return NULL;
    }
    // 从原链表的第一个节点开始遍历
    ListNode *current = originalList->first;
    while (current!= NULL) {
        // 在新链表的尾部插入与原链表当前节点值相同的节点
        insertAtEnd(newList, current->value);
        // 移动到原链表的下一个节点
        current = current->next;
    }
    return newList;
}

// 连接两个链表，将第二个链表连接到第一个链表的尾部
void concatenateLists(List *list1, List *list2) {
    if (list2 == NULL) {
        return;
    }
    if (list1->last == NULL) {
        // 如果list1为空，直接将list2赋值给list1
        list1->first = list2->first;
        list1->last = list2->last;
    } else {
        // 将list2的第一个节点连接到list1的最后一个节点之后
        list1->last->next = list2->first;
        if (list2->first!= NULL) {
            // 如果list2不为空，设置list2第一个节点的prev指针
            list2->first->prev = list1->last;
        }
        // 更新list1的最后一个节点为list2的最后一个节点
        list1->last = list2->last;
    }
    // 清空list2，使其不再指向原来的链表
    list2->first = NULL;
    list2->last = NULL;
}

// 分割链表，在指定值的节点处分割，返回分割后的第二个链表
List* splitList(List *list, int splitValue) {
    List *newList = createList();
    if (newList == NULL) {
        return NULL;
    }
    ListNode *current = list->first;
    while (current!= NULL) {
        if (current->value == splitValue) {
            if (current == list->last) {
                // 如果分割点是原链表的最后一个节点
                newList->first = NULL;
                newList->last = NULL;
                list->last = current->prev;
                if (list->last!= NULL) {
                    list->last->next = NULL;
                }
            } else {
                // 分割链表
                newList->first = current->next;
                newList->first->prev = NULL;
                list->last = current;
                list->last->next = NULL;
            }
            break;
        }
        current = current->next;
    }
    return newList;
}

int main() {
    List *list = createList();
    if (list == NULL) {
        printf("Failed to create list.\n");
        return 1;
    }

    insertAtEnd(list, 1);
    insertAtEnd(list, 2);
    insertAtEnd(list, 3);

    printf("Forward traversal before deletion: ");
    traverseForward(list);

    deleteNode(list, 2);

    printf("Forward traversal after deletion: ");
    traverseForward(list);

    // 测试复制链表
    List *copiedList = copyList(list);
    printf("Copied list: ");
    traverseForward(copiedList);

    // 测试连接链表
    List *listToConcatenate = createList();
    insertAtEnd(listToConcatenate, 4);
    insertAtEnd(listToConcatenate, 5);
    concatenateLists(list, listToConcatenate);
    printf("List after concatenation: ");
    traverseForward(list);

    // 测试分割链表
    List *splitResult = splitList(list, 3);
    printf("Original list after split: ");
    traverseForward(list);
    printf("Split list: ");
    traverseForward(splitResult);

    freeList(list);
    freeList(copiedList);
    freeList(splitResult);
    return 0;
}
