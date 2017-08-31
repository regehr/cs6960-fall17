/* I'm gonna annotate this with my thoughts as I write this.
 * I hope the comments aren't too verbose. */

/* Note: 19:43:24 
 * I'm running into some bug with my writing code and haven't gotten
 * to the read part yet. It's probably somewhere in the code handling
 * when you go over the buffer size. */

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

/* How many bytes of freespace in the pipe before considering a short write. */
#define SHORTW_THRESHOLD 256

/* Stealing your min() from pipespeed.c */
#define min(x,y) (((x)<(y))?(x):(y)) 
#define max(x,y) (((x)<(y))?(y):(x))

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

/* I wish I had time to optimize this, 
 * but Distributed Systems is due tonight too. :C */

/* Cicular Buffer Notes
 * --------------------
 * 
 * Rereading [1] made me finally grok the magic of using modulo for circulars 
 * buffers.
 *
 * - w ~= r: 
 *   - w = r:   empty
 *   - else:     full
 *   
 *
 * [1] https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
 *
 */
int
pipewrite(struct pipe *p, char *addr, int n)
{
  /* I'm a little torn on the semantics of signaling for pipes.
   * Does a reader want to know someone tried writing even through they 
   * wrote zero bytes? I was worried about the reader getting stuck, 
   * but then I suppose that pipeclose should wake them. I guess I'll quickly
   * return on zero.
   * */
  if (n == 0) return 0;

  acquire(&p->lock);

  int freeSpace = PIPESIZE - (p->nwrite - p->nread);

  /* It might cause a lot of switching back and forth between the usercode
   * and the pipe, so maybe it might be better to block rather than short write
   * zero bytes.
   *
   * TODO: It might be worth trying to tune a threshold for how many bytes are
   *       needed to consider a shortwrite.
   *
   *       ^DID
   *
   * TODO: Consider buffer end as another threshold
   *
   * - If n is smaller than the threshold, we can check if we have enough space to write that.
   */
  while (freeSpace < min(SHORTW_THRESHOLD, n) || !freeSpace) {
    if (p->readopen == 0 || myproc()->killed) {
      release(&p->lock);
      return -1;
    }
    wakeup(&p->nread);
    sleep(&p->nwrite, &p->lock);
  }

  int writeIndex = p->nwrite % PIPESIZE;

  /* Check if we're about to touch the buffer boundary. */
  int toEnd = PIPESIZE - writeIndex;

  const int firstWrite = min(min(toEnd, freeSpace), n);

  /* First write up to the buffer end */
  memmove(&p->data[writeIndex], addr, firstWrite);

  p->nwrite += firstWrite;
  freeSpace -= firstWrite;

  /* Prepare second write in case have more than until the buffer end */
  int bytesLeft = n - firstWrite;

  const int secondWrite = min(bytesLeft, freeSpace);

  if (secondWrite) {
    memmove(p->data, &addr[firstWrite], secondWrite);
  }

  wakeup(&p->nread);
  release(&p->lock);
  return firstWrite + secondWrite;
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
