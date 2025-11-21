/*
* @Descripttion: 基于杂项设备的蜂鸣器驱动
*/
#include <linux/init.h> //初始化头文件
#include <linux/module.h> //最基本的文件，支持动态添加和卸载模块。
#include <linux/miscdevice.h> //包含了 miscdevice 结构的定义及相关的操作函数。
#include <linux/fs.h> //文件系统头文件，定义文件表结构（file,buffer_head,m_inode 等）
#include <linux/uaccess.h> //包含了 copy_to_user、copy_from_user 等内核访问用户进程内存地址的函数定义。
#include <linux/io.h> //包含了 ioremap、iowrite 等内核访问 IO 内存等函数的定义。
#include <linux/kernel.h> //驱动要写入内核，与内核相关的头文件

#define GPIO5_DR 0x020AC000 //蜂鸣器物理地址，通过查看原理图得知
unsigned int *vir_gpio5_dr; //存放映射完的虚拟地址的首地址
/**
* @name: misc_read
* @test: 从设备中读取数据，当用户层调用函数 read 时，对应的，内核驱动就会调用这个函数。
* @msg:
* @param {structfile} *file file 结构体
* @param {char__user} *ubuf 这是对应用户层的 read 函数的第二个参数 void *buf
* @param {size_t} size 对应应用层的 read 函数的第三个参数
* @param {loff_t} *loff_t 这是用于存放文件的偏移量的，回想一下系统编程时，读写文件的操
作都会使偏移量往后移。
* @return {*} 当返回正数时，内核会把值传给应用程序的返回值。一般的，调用成功会返回成功
读取的字节数。
如果返回负数，内核就会认为这是错误，应用程序返回-1
*/
ssize_t misc_read (struct file *file, char __user *ubuf, size_t size, loff_t *lo
ff_t)
{
printk("misc_read\n ");
return 0;
}
/**
* @name: misc_write
* @test: 往设备写入数据，当用户层调用函数 write 时，对应的，内核驱动就会调用这个函数。
* @msg:
* @param {structfile} * filefile 结构体
* @param {constchar__user} *ubuf 这是对应用户层的 write 函数的第二个参数
const void *buf
* @param {size_t} size 对应用户层的 write 函数的第三个参数 count。
* @param {loff_t} *loff_t 这是用于存放文件的偏移量的，回想一下系统编程时，读写文件的操
作都会使偏移量往后移。
* @return {*} 当返回正数时，内核会把值传给应用程序的返回值。一般的，调用成功会返回成功
读取的字节数。
如果返回负数，内核就会认为这是错误，应用程序返回-1。
*/
ssize_t misc_write (struct file *file, const char __user *ubuf, size_t size, lof
f_t *loff_t)
{
/*应用程序传入数据到内核空间，然后控制蜂鸣器的逻辑，在此添加*/
// kbuf 保存的是从应用层读取到的数据
char kbuf[64] = {0};
// copy_from_user 从应用层传递数据给内核层
if(copy_from_user(kbuf,ubuf,size)!= 0)
{
// copy_from_user 传递失败打印
printk("copy_from_user error \n ");
return -1;
}
//打印传递进内核的数据
printk("kbuf is %d\n ",kbuf[0]);
if(kbuf[0]==1) //传入数据为 1 ，蜂鸣器响
{
*vir_gpio5_dr |= (1<<1);
}
else if(kbuf[0]==0) //传入数据为 0，蜂鸣器关闭
*vir_gpio5_dr &= ~(1<<1);
return 0;
}
/**
* @name: misc_release
* @test: 当设备文件被关闭时内核会调用这个操作，当然这也可以不实现，函数默认为 NULL。关
闭设备永远成功。
* @msg:
* @param {structinode} *inode 设备节点
* @param {structfile} *file filefile 结构体
* @return {0}
*/
int misc_release(struct inode *inode,struct file *file){
printk("hello misc_relaease bye bye \n ");
return 0;
}
/**
* @name: misc_open
* @test: 在操作设备前必须先调用 open 函数打开文件，可以干一些需要的初始化操作。
* @msg:
* @param {structinode} *inode 设备节点
* @param {structfile} *file filefile 结构体
* @return {0}
*/
int misc_open(struct inode *inode,struct file *file){
printk("hello misc_open\n ");
return 0;
}
//文件操作集
struct file_operations misc_fops={
.owner = THIS_MODULE,
.open = misc_open,
.release = misc_release,
.read = misc_read,
.write = misc_write,
};
//miscdevice 结构体
struct miscdevice misc_dev = {
.minor = MISC_DYNAMIC_MINOR,
.name = "hello_misc",
.fops = &misc_fops,
};
static int misc_init(void)
{
int ret;
//注册杂项设备
ret = misc_register(&misc_dev);
if(ret<0)
{
printk("misc registe is error \n");
}
printk("misc registe is succeed \n");
//将物理地址转化为虚拟地址
vir_gpio5_dr = ioremap(GPIO5_DR,4);
if(vir_gpio5_dr == NULL)
{
printk("GPIO5_DR ioremap is error \n");
return EBUSY;
}
printk("GPIO5_DR ioremap is ok \n");
return 0;
}
static void misc_exit(void){
//卸载杂项设备
misc_deregister(&misc_dev);
iounmap(vir_gpio5_dr);
printk(" misc gooodbye! \n");
}
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");