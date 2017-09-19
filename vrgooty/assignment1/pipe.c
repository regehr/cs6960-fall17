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
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

void*
memcopy(void *dst, const void *src, uint n)
{
	char* dst8 = (char*)dst;
	char* src8 = (char*)src;
	int rem = n%8;
					        
	for(int i=0; i<rem; i++){
		dst8[i] = src8[i];
	}   
	
	dst8 += rem;
	src8 += rem;
		
	n -= rem;
	n /= 8;
	while (n--) {
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		dst8[3] = src8[3];
		dst8[4] = src8[4];
		dst8[5] = src8[5];
		dst8[6] = src8[6];
		dst8[7] = src8[7];
		dst8 += 8;
		src8 += 8;
	}
	return dst;
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
  int i = 0;

  acquire(&p->lock);
  while(i<n){
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }
    //p->data[p->nwrite++ % PIPESIZE] = addr[i];

		int n1 = n-i;
		n1 = n1<=(PIPESIZE - (p->nwrite - p->nread))? n1 : (PIPESIZE - (p->nwrite - p->nread));
		int start_index = p->nwrite%PIPESIZE;
		n1 = n1<=(PIPESIZE-start_index) ? n1 : (PIPESIZE-start_index);
		memcopy(p->data+start_index, addr+i, n1);
		p->nwrite += n1;
		i += n1;
  }
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}

int
piperead(struct pipe *p, char *addr, int n)
{
//  int i;

  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }
  
	/*
			for(i = 0; i < n; i++){  //DOC: piperead-copy
				if(p->nread == p->nwrite)
					break;
				addr[i] = p->data[p->nread++ % PIPESIZE];
			}
	*/
	
	n = n<=(p->nwrite - p->nread) ? n : (p->nwrite - p->nread);
	int start_index = p->nread % PIPESIZE;
	int n1 = 0;
	if(start_index + n > PIPESIZE){
		n1 = PIPESIZE - start_index;
		memcopy(addr, p->data+start_index, n1);
		p->nread += n1;
		addr += n1;
		n -= n1;
		start_index = p->nread % PIPESIZE;
	}
	memcopy(addr, p->data+start_index, n);
	p->nread += n;

  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return n+n1;
}
