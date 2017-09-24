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

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
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

#if 1
void printArraySlice(char *data, char *data2, int len)
{
  int i;
  for(i = 0; i < len; ++i){
    cprintf("%d ", data[i]);
  }
  cprintf(" : ");
  for(i = 0; i < len; ++i){
    cprintf("%d ", data2[i]);
  }
  cprintf("\n");
}
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
//PAGEBREAK: 40
int
pipewrite(struct pipe *p, char *addr, int n)
{
  //int i;
  int bytes_written = 0;
  int bytes_to_write;

  acquire(&p->lock);
  while(bytes_written < n) {
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }
    bytes_to_write = n - bytes_written;
    
    //NOTE: my code had at least one off-by-one error (or was entirely
    //erroneous), so I resorted to using Alex Steele's code here. My write code
    //is in the comment below this one. If I have time, I'll try to further improve
    //the performance or try an alternate implementation, but for now I'm handing this in.
    /*
     * int write_length = MIN(bytes_to_write, (PIPESIZE - (p->nwrite % PIPESIZE)));
     * memmove(p->data + (p->nwrite % PIPESIZE), (addr + bytes_written), write_length);
     * p->nwrite += write_length;
     * bytes_written += write_length;
     */
    if(PIPESIZE + p->nread - p->nwrite < bytes_to_write){
      bytes_to_write = PIPESIZE + p->nread - p->nwrite;
    }
    if(PIPESIZE - (p->nwrite % PIPESIZE) < bytes_to_write){
      bytes_to_write = PIPESIZE - (p->nwrite % PIPESIZE);
    }
    memmove(p->data + (p->nwrite % PIPESIZE), (addr + bytes_written), bytes_to_write);
    p->nwrite += bytes_to_write;
    bytes_written += bytes_to_write;
  }

  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}

int
piperead(struct pipe *p, char *addr, int n)
{
  //Because I got stuck on an error in the write code, I did not yet have
  //functioning read code. The code below is, once again, Alex Steele's.
  int nr;
  int x;

  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }
  nr = n;
  if (p->nwrite - p->nread < nr) {
    nr = p->nwrite - p->nread;
  }
  if (PIPESIZE - (p->nread % PIPESIZE) < nr) {
    nr = PIPESIZE - (p->nread % PIPESIZE);
  }
  memmove(addr, p->data + (p->nread % PIPESIZE), nr);
  p->nread += nr;
  if (nr < n && p->nread < p->nwrite) {
    x = n - nr;
    if (p->nwrite - p->nread < x) {
      x = p->nwrite - p->nread;
    }
    memmove(addr + nr, p->data + (p->nread % PIPESIZE), x);
    p->nread += x;
    nr += x;
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return nr;
}
