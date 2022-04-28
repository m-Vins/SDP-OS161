#include <types.h>
#include <lib.h>
#include <syscall.h>
#include <kern/unistd.h> //to include STDOUT_FILENO and STDERR_FILENO




size_t sys_write(int fd, const void *buf, size_t count){
    unsigned int i = 0;
    char *b = (char *)buf;

    if(fd == STDOUT_FILENO || fd == STDERR_FILENO){
        for(i = 0; i < (unsigned int)count; i++){
            putch(b[i]);   
        }
    }
    return i;
}


size_t sys_read(int fd, const void *buf, size_t count){
    unsigned int i = 0;
    if(fd == 0){
        for(i = 0; i < (unsigned int)count; i++){
            ((char *)buf)[i] = getch();
        }
    }
    return i;
}