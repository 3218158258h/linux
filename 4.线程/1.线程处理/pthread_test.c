#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
typedef struct {
        int num1;
        int num2;
    }num_def;
void *small(void *args)
{
    num_def * num = (num_def *)args;
    if(num->num1>=10||num->num2>=10)
    {
        printf("数字太大辣，让老大算\n");
        return NULL;
    }
    else{
        int *result=malloc(sizeof(int));
        *result=num->num1+num->num2;
        printf("Result:%d+%d=%d\n",num->num1,num->num2,num->num1+num->num2);
        return (void *)result;
    }
}
void *big(void *args)
{
    num_def * num = (num_def *)args;

    if(num->num1<10&&num->num2<10)
    {
        printf("数字太小了，本老大懒得算\n");
        return NULL;
    }
    else{
        int *result=malloc(sizeof(int));
        *result=num->num1+num->num2;
        printf("Result:%d+%d=%d\n",num->num1,num->num2,num->num1+num->num2);
        return (void *)result;
    }
}

int main() {
    int *result_big=NULL,*result_small=NULL;
    pthread_t tid_small,tid_big;
    num_def *num=(num_def*)malloc(sizeof(num_def));

    printf("请输入两个数字");
    scanf("%d %d",&num->num1,&num->num2);

    pthread_create(&tid_small,NULL,(void *)small,(void *)num);
    pthread_create(&tid_big,NULL,(void *)big,(void *)num);

    pthread_join(tid_small,(void**)&result_small);
    pthread_join(tid_big,(void**)&result_big);
    if(result_small != NULL)
    {
        printf("%d\n",*result_small);
    }

    if(result_big != NULL)
    {
        printf("%d\n",*result_big);
    }
    
    free(num);
    if (result_small != NULL) free(result_small);
    if (result_big != NULL) free(result_big);
    return 0;
}
