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
    if((fd = shm_open(shm_file,O_RDWR,0666))==-1)
    {
        perror("shm_open");
        exit(1);
    }
    if((share=mmap(NULL,2048,PROT_READ,MAP_SHARED,fd,0))==MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    printf("%s\n", share);
    close(fd);
    munmap(share,2048);
    shm_unlink(shm_file);
    return 0;
}
