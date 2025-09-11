#include <stdio.h>
void bubblesort(int a[],int n)
{
    int t;
    for(int i=0;i<n-1;i++){
        for(int j=0;j<n-1-i;j++){
            if(a[j]<a[j+1]){
                t=a[j+1];
                a[j+1]=a[j];
                a[j]=t;
            }
        }
    }
}
int main()
{
    int a[6];
    printf("请输入6位数字：");
    for(int i=0;i<=5;i++)
    {
        scanf("%d",&a[i]);
    }
    bubblesort(a,6);
    for(int i=0;i<=5;i++)
    {
        printf("%d ",a[i]);
    }

return 0;
}
