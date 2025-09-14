#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <mqueue.h>
#include <time.h>
int main() {
    mqd_t mqdes;
    char *mq_file="/mqtest";

    struct mq_attr mq_addr;
        mq_addr.mq_flags=0;
        mq_addr.mq_maxmsg=5;
        mq_addr.mq_msgsize=100;
        mq_addr.mq_curmsgs=0;

    if((mqdes=mq_open(mq_file,O_CREAT|O_WRONLY,0664,&mq_addr))== -1){
        perror("mq_open");
        exit(1);
    }
    char buf[1024];
    ssize_t count;
    for(int t=0;t<=5&&(count=read(0,buf,100))>0;t++)
    {
        if (count > 100) {
            printf("too big message\n");
            exit(1);
        }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME,&ts);
        ts.tv_sec+= 5;
        if(mq_timedsend(mqdes,buf,count,0,&ts)==-1)
        {
            perror("mq_timedsend");
            exit(1);
        }
    }
    mq_close(mqdes);
    return 0;
}
