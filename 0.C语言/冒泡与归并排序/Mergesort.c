#include <stdio.h>
#include <stdlib.h>
void merge(int arr[],int left,int mid,int right)
{
    int l1=mid+1-left,l2=right-mid;
    int a[l1],b[l2];
    for(int i=0;i<l1;i++)
    {
        a[i]=arr[left+i];
    }
    for(int i=0;i<l2;i++)
    {
        b[i]=arr[mid+i+1];
    }
    int i=0,j=0;
    while(i<l1&&j<l2)
    {
    if(a[i]<=b[j])
        {
            arr[left]=a[i];
            i++;
        }
    else
        {
            arr[left]=b[j];
            j++;
        }
        left++;
    }
    while(i<l1)
    {
        arr[left]=a[i];
        left++;
        i++;
    }
    while(j<l2)
    {
        arr[left]=b[j];
        left++;
        j++;
    }
}
void mergesort(int arr[],int left,int right)//归并
{
    if(left<right)//判断是否分割为最小单位
    {
    int mid=left+(right-left)/2;
    mergesort(arr,left,mid);
    mergesort(arr,mid+1,right);
    merge(arr,left,mid,right);
    }

}
int main()
{
    int *a;
    int n;
    printf("要输入的数字有多少个:");
    scanf("%d",&n);
    a=(int*)malloc(n*sizeof(int));
    printf("请输入：");
    for(int i=0;i<n;i++)
    {
        scanf("%d",&a[i]);
    }
    printf("排序前的数组为: ");
    for(int i=0;i<n;i++)
    {
        printf("%d ",a[i]);
    }
    mergesort(a,0,n-1);
    printf("\n排序后的数组为: ");
    for(int i=0;i<n;i++)
    {
        printf("%d ",a[i]);
    }
    free(a);
    return 0;
}
