#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <syscall.h>
#include <kern/unistd.h> //to include STDOUT_FILENO and STDERR_FILENO

#if OPT_FILESYSCALL == 1
#include <copyinout.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include <current.h>

#define SYSTEM_OPEN_MAX (10*PROC_MAX_FILE)



struct openfile systemFileTable[SYSTEM_OPEN_MAX];
#endif

#if OPT_FILESYSCALL == 1
int
sys_open(userptr_t path, int openflags, mode_t mode, int *errp)
{
  int fd, i;
  struct vnode *v;
  struct openfile *of=NULL;; 	
  int result;

  result = vfs_open((char *)path, openflags, mode, &v);
  if (result) {
    *errp = ENOENT;
    return -1;
  }

  /* search system open file table */
  for (i=0; i<SYSTEM_OPEN_MAX; i++) {
    if (systemFileTable[i].vn==NULL) {
      of = &systemFileTable[i];
      of->vn = v;
      of->offset = 0; // TODO: handle offset with append
      of->countRef = 1;
      break;
    }
  }

  if (of==NULL) { 
    // no free slot in system open file table
    *errp = ENFILE;
  }
  else {
    for (fd=STDERR_FILENO+1; fd<OPEN_MAX; fd++) {
      if (curproc->fileTable[fd] == NULL) {
	curproc->fileTable[fd] = of;
	return fd;
      }
    }
    // no free slot in process open file table
    *errp = EMFILE;
  }
  
  vfs_close(v);
  return -1;

}

/*
 * file system calls for open/close
 */
int
sys_close(int fd)
{
  struct openfile *of=NULL; 
  struct vnode *vn;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  curproc->fileTable[fd] = NULL;

  if (--of->countRef > 0) return 0; // just decrement ref cnt
  vn = of->vn;
  of->vn = NULL;
  if (vn==NULL) return -1;

  vfs_close(vn);	
  return 0;
}

static int
file_write(int fd, userptr_t buf_ptr, size_t size){
    struct openfile *of;
    struct iovec iov;
    struct vnode *v;
	struct uio ku;
    int result;
    void *kbuf;

        
    of = curproc->fileTable[fd];
    if(of == NULL ) return -1;

    v = of->vn;    
    kbuf = kmalloc(size);
    copyin(buf_ptr,kbuf,size);
    uio_kinit(&iov, &ku, kbuf, size, 0, UIO_WRITE);

    result = VOP_WRITE(v, &ku);
    if (result) {
        return result;
    }

    kfree(kbuf);
    of->offset = ku.uio_offset;
    return (size - ku.uio_resid);
}

static int
file_read(int fd, userptr_t buf_ptr, size_t size) {
    struct openfile *of;
    struct iovec iov;
    struct vnode *v;
	struct uio ku;
    int result;
    void *kbuf;
    
    of = curproc->fileTable[fd];
    if(of == NULL ) return -1;

    v = of->vn;    
    kbuf = kmalloc(size);
    uio_kinit(&iov, &ku, kbuf, size, 0, UIO_READ);

    result = VOP_READ(v, &ku);
    if (result) {
        return result;
    }

    copyout(kbuf,buf_ptr,size);
    kfree(kbuf);
    of->offset = ku.uio_offset;
    return (size - ku.uio_resid);
}


#endif





size_t sys_write(int fd, const void *buf, size_t count){
    unsigned int i = 0;
    char *b = (char *)buf;

  if (fd!=STDOUT_FILENO && fd!=STDERR_FILENO) {
#if OPT_FILESYSCALL == 1
    return file_write(fd, (userptr_t)buf, count);
#else
    kprintf("sys_write supported only to stdout\n");
    return -1;
#endif
  }
    if(fd == STDOUT_FILENO || fd == STDERR_FILENO){
        for(i = 0; i < (unsigned int)count; i++){
            putch(b[i]);   
        }
    }
    return i;
}


size_t sys_read(int fd, const void *buf, size_t count){
    unsigned int i = 0;


  if (fd!=STDIN_FILENO) {
#if OPT_FILESYSCALL == 1
    return file_read(fd, (userptr_t)buf, count);
#else
    kprintf("sys_read supported only to stdin\n");
    return -1;
#endif
  }

    if(fd == 0){
        for(i = 0; i < (unsigned int)count; i++){
            ((char *)buf)[i] = getch();
        }
    }
    return i;
}
