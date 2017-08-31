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
  char data[PIPESIZE];
  void *buf_end;
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
  // Place a pointer to the end of the buffer so we know when to wrap.
  p->buf_end = (void *) &p->data + PIPESIZE;
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

  // Determine how much data we can write.
  int buffer_space = PIPESIZE - (p->nwrite - p->nread);

  int num_to_write = n;
  if (num_to_write > buffer_space) {
      num_to_write = buffer_space;
  }

  int write_position = p->nwrite % PIPESIZE;
  int read_position = p->nread % PIPESIZE;

  // If the space is "outside" of the two positions (write to right of read)
  if (write_position >= read_position) {
    // Determine the amount of data we can write before we wrap.
    int cont_space = p->buf_end - ((void *) p->data + write_position);
    if (num_to_write < cont_space) {
      cont_space = num_to_write;
    }
    
    // Write the data up to the end of the buffer (or the end of data)
    memmove(p->data + write_position, addr, cont_space);

    // Copy any additional bytes at the beginning of the buffer.
    int remaining_to_copy = num_to_write - cont_space;
    if (remaining_to_copy > 0) {
      memmove(p->data, addr + cont_space, remaining_to_copy);
    }

  // If the space is "inside" of the two positions (write to left of read)
  } else {
    memmove(p->data + write_position, addr, num_to_write);
  }

  p->nwrite += num_to_write;
    
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return num_to_write;
}

int
piperead(struct pipe *p, char *addr, int n)
{
  int i;

  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(p->nread == p->nwrite)
      break;
    addr[i] = p->data[p->nread++ % PIPESIZE];
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return i;
}
