#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct pipe {
  char data[PIPESIZE];	//For alignment
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

void*
my_memcpy(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;

	for (int i = 0; i < n; ++i){
    	d[i] = s[i];
	}
   return d;
}

//PAGEBREAK: 40
int
pipewrite(struct pipe *p, char *addr, int n)
{
  int i;

  acquire(&p->lock);
  for(i = 0; i < n; ){
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }


	//Simplest idea explained in class

	//How much be written in current state
	uint size = MIN(PIPESIZE - (p->nwrite - p->nread), (uint)(n - i));
    //Determine if we need a split
	uint actSize = MIN(size, PIPESIZE - p->nwrite%PIPESIZE);
	
    my_memcpy(&p->data[p->nwrite%PIPESIZE], &addr[i], actSize);
    if(actSize!=size)my_memcpy(p->data, &addr[i + actSize], size - actSize);

    p->nwrite += size;
    i += size;
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
 
	//Since short reads are present, we don't need to have a for loop at all and can perform all at once
	uint size = MIN(p->nwrite - p->nread, (uint)n);
  	uint actSize = MIN(size, PIPESIZE - p->nread%PIPESIZE);

  	my_memcpy(addr, &p->data[p->nread%PIPESIZE], actSize);
  	if(actSize!=size)my_memcpy(&addr[actSize], p->data, size - actSize);

  	p->nread += size;
 
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return size;
}
