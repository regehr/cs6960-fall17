#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"

#define NMUTEX 10

struct mutex {
  struct sleeplock lk;
  int used;
};

struct { 
  struct spinlock lk;
  struct mutex mutexes[NMUTEX];
} mtable;

// api design and implementation is self made
// some of the details were helped by looking at
// alex-steele's code

void
minit(void)
{
  int i;

  initlock(&mtable.lk, "mtable");

  for (i = 0; i < NMUTEX; i++)
  {
    initsleeplock(&mtable.mutexes[i].lk, "mutex");
    mtable.mutexes[i].used = 0;
  }
}

int
mutex_create(void)
{
  int i;

  acquire(&mtable.lk);
  for (i = 0; i < NMUTEX; i++)
  {
    if (mtable.mutexes[i].used == 0)
    {
      mtable.mutexes[i].used = 1;
      release(&mtable.lk);
      return i;
    }
  }
  release(&mtable.lk);
  return -1;
}

int
mutex_acquire(int i)
{
  if (i < 0 || i > NMUTEX || mtable.mutexes[i].used == 0)
  {
    return -1;
  }
  acquiresleep(&mtable.mutexes[i].lk);
  return 0;
}

int
mutex_release(int i)
{
  if (i < 0 || i > NMUTEX || mtable.mutexes[i].used == 0)
  {
    return -1;
  }
  releasesleep(&mtable.mutexes[i].lk);
  return 0;
}

int
mutex_destroy(int i)
{
  if (i < 0 || i > NMUTEX || mtable.mutexes[i].used == 0)
  {
    return -1;
  }
  acquire(&mtable.lk);
  mtable.mutexes[i].used = 0;
  release(&mtable.lk);
  return 0;
}
