#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 2048

struct pipe {
  char data[PIPESIZE];
  struct spinlock lock;
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

void*
memcpy(void *dst, const void *src, uint n);

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
  int i;
  
    acquire(&p->lock);
  
    i = 0;
    while (i < n) {
      while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
        if(p->readopen == 0 || myproc()->killed){
          release(&p->lock);
          return -1;
        }
        wakeup(&p->nread);
        sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
      }
      
      int bytes_to_write;
      if (PIPESIZE - (p->nwrite - p->nread) < n - i)
        bytes_to_write = PIPESIZE - (p->nwrite - p->nread);
      else bytes_to_write = n - i;
      int bytes_to_end = PIPESIZE - (p->nwrite % PIPESIZE);
  
      if (bytes_to_end > bytes_to_write) {
        memcpy(p->data + (p->nwrite % PIPESIZE), addr, bytes_to_write);
      } else {
        memcpy(p->data + (p->nwrite % PIPESIZE), addr, bytes_to_end);
        memcpy(p->data, addr + bytes_to_end, bytes_to_write - bytes_to_end);
      }
      
      p->nwrite += bytes_to_write;
      i += bytes_to_write;
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
  int bytes_to_read;
  if (p->nwrite - p->nread < n)
    bytes_to_read = p->nwrite - p->nread;
  else bytes_to_read = n;
  int bytes_to_end = PIPESIZE - (p->nread % PIPESIZE);

  if (bytes_to_end > bytes_to_read) {
    memcpy(addr, p->data + p->nread % PIPESIZE, bytes_to_read);
  } else {
    memcpy(addr, p->data + p->nread % PIPESIZE, bytes_to_end);
    memcpy(addr + bytes_to_end, p->data, bytes_to_read - bytes_to_end);
  }
  
  p->nread += bytes_to_read;
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return bytes_to_read;
}
