#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 4096

// Taken from: https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

struct pipe {
  char *data; // Decided to switch this to be dynamically allocated after seeing it in class.
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
  p->data = kalloc();
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
    kfree(p->data);
    kfree((char*)p);
  } else
    release(&p->lock);
}

//PAGEBREAK: 40
int
pipewrite(struct pipe *p, char *addr, int n)
{
  acquire(&p->lock);

  int i = 0;
  while (i < n) {
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }

    // Figure out how many bytes we'll write total.
    int buf_space = PIPESIZE - (p->nwrite - p->nread);
    int num_to_write = MIN(buf_space, n - i);

    // Figure out the current position at which we'll start writing data.
    int write_position = p->nwrite % PIPESIZE;

    // Figure out how many bytes we should write before the buffer wraps.
    int end_space = PIPESIZE - write_position;
    int end_write_amount = MIN(num_to_write, end_space);

    memmove(&p->data[write_position], &addr[i], end_write_amount);

    // Figure out how many bytes we have left to write at the beginning.
    int begin_amount = num_to_write - end_write_amount;

    if (begin_amount) {
      memmove(p->data, &addr[i + end_write_amount], begin_amount);
    }

    i += num_to_write;
    p->nwrite += num_to_write;
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

  // Figure out how many bytes we'll read total.
  int amount_in_pipe = p->nwrite - p->nread;
  int num_to_read = MIN(amount_in_pipe, n);

  // Figure out the current position at which we'll start reading data.
  int read_position = p->nread % PIPESIZE;

  // Figure out how many bytes we should read before the buffer wraps.
  int end_space = PIPESIZE - read_position;
  int end_amount_in_pipe = MIN(num_to_read, end_space);

  memmove(addr, &p->data[read_position], end_amount_in_pipe);

  // Figure out how many bytes we have left to read.
  int begin_amount = num_to_read - end_amount_in_pipe;

  if (begin_amount) {
    memmove(&addr[end_amount_in_pipe], p->data, begin_amount);
  }

  p->nread += num_to_read;

  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);

  return num_to_read;
}
