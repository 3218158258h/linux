#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
int main() {
    char *shm_file="/test1293";
    char *share;
    int fd;
    if((fd = shm_open(shm_file,O_RDWR | O_CREAT,0666))==-1)
    {
        perror("shm_open");
        exit(1);
    }
    if(ftruncate(fd,2048)==-1)
    {
        perror("ftruncate");
        close(fd);
        shm_unlink(shm_file);
        exit(1);
    }
    if((share=mmap(NULL,2048,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0))==MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    ssize_t count=0;
    ssize_t b_read=0;
    do{
        count = read(0,share+b_read,2048-b_read);
        if(count == -1){
            perror("read");
            exit(1);
        }
        else{
            b_read+=count;
        }
    }
    while(count >0);
    if (b_read < 2048) {
        share[b_read] = '\0';
    }
    sleep(5);
    close(fd);
    munmap(share,2048);
    return 0;
}
