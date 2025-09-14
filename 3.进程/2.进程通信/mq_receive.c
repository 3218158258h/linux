#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <mqueue.h>
int main() {
    mqd_t mqdes;
    char *mq_file="/mqtest";

    struct mq_attr mq_addr;
        mq_addr.mq_flags=0;
        mq_addr.mq_maxmsg=5;
        mq_addr.mq_msgsize=100;
        mq_addr.mq_curmsgs=0;

    if((mqdes=mq_open(mq_file,O_RDONLY,0664,&mq_addr))== -1){
        perror("mq_open");
        exit(1);
    }
    
    for(int t=0;t<=5;t++)
    {
        char buf[1024];
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME,&ts);
        ts.tv_sec+= 5;
        ssize_t b_recieve=mq_timedreceive(mqdes,buf,sizeof(buf),0,&ts);
        if(b_recieve==-1)
        {
            perror("mq_timedreceive");
            exit(1);
        }
        buf[b_recieve]='\0';
        printf("%s",buf);
    }
    close(mqdes);
    mq_unlink(mq_file);
    return 0;
}

