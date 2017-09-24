#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

// FROM: mmu.h
//    page granularity - aligned chunks of 4096 (2^12) bytes
#define PIPESIZE 4096

// using MIN & MAX from from provided pipespeed.c source
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)<(y))?(y):(x))

// Use xv6 memcpy for now
void*
memcpy(void * restrict dst, const void * restrict src, uint n);

struct pipe {

  // achieve page alignment for data - pack struct w/ data first
  char   * data;

  struct  spinlock lock;
  uint    nread;           // number of bytes read
  uint    nwrite;          // number of bytes written
  int     readopen;        // read fd is still open
  int     writeopen;       // write fd is still open
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
  if (((p->data) = kalloc()) == 0)
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
 if(p && p->data)
   kfree(p->data);
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
    kfree(p->data);
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
  for(i = 0; i < n;){
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }

    // previous, slow approach to write
    //----------------------------------------------------------
//    p->data[p->nwrite++ % PIPESIZE] = addr[i];


    //----------------------------------------------------------
    // simplest approach I could see for this, write in two chunks
    // ... parts of this idea borrowed from Pavol (same lab)
    uint count = MIN(PIPESIZE - (p->nwrite - p->nread), (uint)(n - i));
    uint end   = MIN(count, PIPESIZE - ((p->nwrite) % PIPESIZE));

    // write in two parts
    memcpy(&p->data[(p->nwrite) % PIPESIZE], &addr[i], end);
    memcpy(p->data, &addr[i + end], count - end);

    p->nwrite += count;
    i += count;
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

  //----------------------------------------------------------
  // previous, slow approach to read
//  int i;
//  for(i = 0; i < n; i++){  //DOC: piperead-copy
//      if(p->nread == p->nwrite)
//          break;
//      addr[i] = p->data[p->nread++ % PIPESIZE];
//  }

  //----------------------------------------------------------
  // simplest approach I could see for this, read in two chunks
  // ... parts of this idea borrowed from Pavol (same lab)
  uint count = MIN(p->nwrite - p->nread, (uint)n);
  uint end   = MIN(count, PIPESIZE - ((p->nread) % PIPESIZE));

  // read in two parts
  memcpy(addr, &p->data[(p->nread) % PIPESIZE], end);
  memcpy(&addr[end], p->data, count - end);

  p->nread += count;

  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return count;
}

