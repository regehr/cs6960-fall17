#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 3516

void my_memcpy(void *dest, const void *src, unsigned int count)
{
    const unsigned int TYPE_WIDTH = sizeof(unsigned int);
    unsigned char* dst8 = (unsigned char*)dest;
    unsigned char* src8 = (unsigned char*)src;

    if ( ((unsigned int)dst8 & (TYPE_WIDTH - 1)) == 0 &&
      ((unsigned int)src8 & (TYPE_WIDTH - 1)) == 0 ) {
      unsigned int* d = (unsigned int*)dest;
      unsigned int* s = (unsigned int*)src;
      for (int i=0; i<count/TYPE_WIDTH; i++) {
        d[i] = s[i];
      }

      dst8 += ((count/TYPE_WIDTH) * TYPE_WIDTH);
      src8 += ((count/TYPE_WIDTH) * TYPE_WIDTH);
      count &= (TYPE_WIDTH - 1);
    }

    memmove(dst8, src8, count);
}

struct pipe {
  char data[PIPESIZE];
  struct spinlock lock;
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *p;

  p = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((p = (struct pipe*)kalloc()) == 0)
    goto bad;
  p->readopen = 1;
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  initlock(&p->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = p;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = p;
  return 0;

//PAGEBREAK: 20
 bad:
  if(p)
    kfree((char*)p);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *p, int writable)
{
  acquire(&p->lock);
  if(writable){
    p->writeopen = 0;
    wakeup(&p->nread);
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if(p->readopen == 0 && p->writeopen == 0){
    release(&p->lock);
    kfree((char*)p);
  } else
    release(&p->lock);
}

//PAGEBREAK: 40
int
pipewrite(struct pipe *p, char *addr, int n)
{
  acquire(&p->lock);

  int nwritten = 0;
  int available_write_len, write_step_len;

  while(nwritten < n) {
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }

    available_write_len = PIPESIZE - (p->nwrite - p->nread);

    write_step_len = (n - nwritten) > available_write_len ? available_write_len : (n - nwritten);

    if( (p->nwrite % PIPESIZE) + write_step_len > PIPESIZE) {
      int head_n = (p->nwrite % PIPESIZE) + write_step_len - PIPESIZE;
      int tail_n = write_step_len - head_n;
      my_memcpy((void*)(p->data + (p->nwrite % PIPESIZE)), (void*)(addr+nwritten), tail_n);
      my_memcpy((void*)(p->data), (void*)(addr+nwritten+tail_n), head_n);
    } else {
      my_memcpy((void*)(p->data + (p->nwrite % PIPESIZE)), (void*)(addr+nwritten), write_step_len);
    }

    p->nwrite += write_step_len;
    nwritten += write_step_len;
  }

  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}

int
piperead(struct pipe *p, char *addr, int n)
{
  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }

  int available_read_len = p->nwrite - p->nread;

  n = n > available_read_len ? available_read_len : n;

  if( (p->nread % PIPESIZE) + n > PIPESIZE) {
    int head_n = (p->nread % PIPESIZE) + n - PIPESIZE;
    int tail_n = n - head_n;
    my_memcpy((void*)addr, (void*)(p->data + (p->nread % PIPESIZE)), tail_n);
    my_memcpy((void*)(addr+tail_n), (void*)(p->data), head_n);
  } else {
    my_memcpy((void*)addr, (void*)(p->data + (p->nread % PIPESIZE)), n);
  }
  p->nread += n;

  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return n;
}
