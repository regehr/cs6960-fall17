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


// memcpy borrowed from open source apple https://opensource.apple.com/source/xnu/xnu-2050.18.24/libsyscall/wrappers/memcpy.c
typedef int word;
#define wsize sizeof(word)
#define wmask (wsize - 1)

void*
mymemcpy(void *dst0, const void *src0, uint length)
{
  char *dst = dst0;
  const char *src = src0;
  uint t;

  if (length == 0 || dst == src)
    goto done;

#define TLOOP(s) if (t) TLOOP1(s)
#define TLOOP1(s) do { s; } while (--t)

  if ((unsigned long)dst < (unsigned long)src) {
    // copy forward
    t = (uint)src;
    if ((t | (uint)dst) & wmask) {
      if ((t ^ (uint)dst) & wmask || length < wsize)
        t = length;
      else
        t = wsize - (t & wmask);
      length -= t;
      TLOOP1(*dst++ = *src++);
    }
    t = length / wsize;
    TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
    t = length & wmask;
    TLOOP(*dst++ = *src++);
  } else {
    src += length;
    dst += length;
    t = (uint)src;
    if ((t | (uint)dst) & wmask) {
      if ((t ^ (uint)dst) & wmask || length <= wsize)
        t = length;
      else
        t &= wmask;
      length -= t;
      TLOOP1(*--dst = *--src);
    }
    t = length / wsize;
    TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
    t = length & wmask;
    TLOOP(*--dst = *--src);
  }
done:
  return (dst0);
}

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
  while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
    if(p->readopen == 0 || myproc()->killed){
      release(&p->lock);
      return -1;
    }
    wakeup(&p->nread);
    sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
  }
  // handle short writes
  int sizeAvailable = (p->nread + PIPESIZE) - p->nwrite;
  if(n > sizeAvailable) {
    n = sizeAvailable;
  }
  int sizeToWrite = PIPESIZE - (p->nwrite % PIPESIZE);
  if (n < sizeToWrite) {
    sizeToWrite = n;
  }
  mymemcpy(&p->data[p->nwrite % PIPESIZE], addr, sizeToWrite);
  p->nwrite = p->nwrite + sizeToWrite;
  if (n > sizeToWrite) {
    addr = addr + sizeToWrite;
    sizeToWrite = n - sizeToWrite;
    mymemcpy(&p->data[p->nwrite % PIPESIZE], addr, sizeToWrite);
    p->nwrite = p->nwrite + sizeToWrite;
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
  int sizeAvailable = p->nwrite - p->nread;
  if(n > sizeAvailable) {
    n = sizeAvailable;
  }
  int sizeToWrite = PIPESIZE - (p->nread % PIPESIZE);
  if (n < sizeToWrite) {
    sizeToWrite = n;
  }
  mymemcpy(addr, &p->data[p->nread % PIPESIZE], sizeToWrite);
  p->nread = p->nread + sizeToWrite;
  if (n > sizeToWrite) {
    addr = addr + sizeToWrite;
    sizeToWrite = n - sizeToWrite;
    mymemcpy(addr, &p->data[p->nread % PIPESIZE], sizeToWrite);
    p->nread = p->nread + sizeToWrite;
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return n;
}
