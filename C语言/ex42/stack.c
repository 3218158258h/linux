#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//定义栈节点
typedef struct stacknode{
    int data;
    struct stacknode *next;
}stacknode;

//创建
stacknode *create(int x)
{
    stacknode *new_node=(stacknode*)malloc(sizeof(stacknode));
    if (new_node == NULL) {
        printf("内存分配失败\n");
        return NULL;
    }
    new_node->data=x;
    new_node->next=NULL;
    return new_node;
}

//压入
stacknode *push(stacknode *head,int data)
{
    stacknode *new_node=create(data);
    if(head==NULL)
    {
        head=new_node;
    }
    else
    {
        new_node->next=head;
        head=new_node;
    }
    return head;
}

//压出
stacknode *pop(stacknode *head)
{
    if(head==NULL)
    {
        printf("栈为空1\n");
    return NULL;
    }
    else
    {
        stacknode *p=head;
        head=head->next;
        free(p);
    }
    return head;
}

//清空
stacknode *clean(stacknode *head)
{
    if(head==NULL)
    {
        printf("栈为空2\n");
    return NULL;
    }
    else{
        while(head!=NULL)
        {
        stacknode *p=head;
        head=head->next;
        free(p);
        }
    }
    return NULL;
}

//打印
void printnode(stacknode *head)
{
    stacknode *p=head;
    if(p==NULL)
    {
        printf("栈为空\n");
    }
    else{
        while(p!=NULL)
        {
        printf("%d ",p->data);
        p=p->next;
        }
        printf("\n");
    }
}
int main() {
    stacknode *head = NULL;
    int num1,num2;
    printf("请输入要压入的数字个数：\n");
    scanf("%d",&num1);
    printf("请输入数字（回车结束）：\n");
    for(int i=1;i<=num1;i++)
    {
    int data;
    scanf("%d", &data);
    head = push(head, data);
    }
    printf("已压入的数字有：");
    printnode(head);
    printf("请输入要压出的数字个数：");
    scanf("%d",&num2);
    for(int i=1;i<=num2;i++)
    {
    head = pop(head);
    }
    printf("剩余的数字有：");
    printnode(head);
    head = clean(head);
    printf("已清空所有数字\n");
    return 0;
}
