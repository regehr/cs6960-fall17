#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE PGSIZE // what kalloc gives us

struct pipe {
  struct spinlock lock;
  char *data;
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
  char *buffer = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((p = (struct pipe*)kalloc()) == 0)
    goto bad;
  if ((buffer = kalloc()) == 0)
    goto bad;
  p->data = buffer;
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
  if(buffer)
    kfree(buffer);
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

char *
s_memcpy(char *const restrict dst, char const *const restrict src, int const count)
{
        for (int i = 0; i < count; ++i)
                dst[i] = src[i];
        return dst;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

//PAGEBREAK: 40
/* no short writes */
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
    /* copy as much as we can; same-ish as in piperead */
    uint const count = MIN(PIPESIZE - (p->nwrite - p->nread), (uint)(n - i));
    uint const end_count = MIN(count, PIPESIZE - p->nwrite%PIPESIZE);

    s_memcpy(&p->data[p->nwrite%PIPESIZE], &addr[i], end_count);
    s_memcpy(p->data, &addr[i + end_count], count - end_count);

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
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty; thanks TAJO
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }

  uint const count = MIN(p->nwrite - p->nread, (uint)n);
  uint const end_count = MIN(count, PIPESIZE - p->nread%PIPESIZE);

  s_memcpy(addr, &p->data[p->nread%PIPESIZE], end_count);
  s_memcpy(&addr[end_count], p->data, count - end_count);

  p->nread += count;

  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return count;
}
