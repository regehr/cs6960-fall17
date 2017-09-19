#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"

#include "mutex.h"


struct {
  struct mutex mutexes[NMTX];
  struct spinlock lock;
} mutex_table;

void 
minit(void)
{
  initlock(&mutex_table.lock, "mutex table");
}

int
mutex_create(int tag)
{
  int i;
  //TAG_UNALLOC is reserved to mean "unallocated": any other integer is a valid tag
  if(tag == TAG_UNALLOC){
    return -1;
  }
  acquire(&mutex_table.lock);
  for(i = 0; i < NMTX; ++i){
    if(mutex_table.mutexes[i].tag == TAG_UNALLOC){
      mutex_table.mutexes[i].tag = tag;
      initlock(&(mutex_table.mutexes[i].lock), "mutex");
      release(&mutex_table.lock);
      return 0;
    }
  }
  //Couldn't find any free space in the mutex table
  release(&mutex_table.lock);
  return -1;
}

int
mutex_acquire(int tag)
{
  int i;
  struct mutex *m;
  if(tag == TAG_UNALLOC){
    return -1;
  }
  acquire(&mutex_table.lock);
  for(i = 0; i < NMTX; ++i){
    if(mutex_table.mutexes[i].tag == tag)
      break;
  }
  //Tag didn't exist
  if(i == NMTX){
    release(&mutex_table.lock);
    return -1;
  }

  //acquire lock
  m = &(mutex_table.mutexes[i]);
  acquire(&m->lock);
  while (m->locked) {
    release(&mutex_table.lock);
    sleep(m, &m->lock);
    acquire(&mutex_table.lock);
  }
  m->locked = 1;
  m->pid = myproc()->pid;
  release(&m->lock);
  release(&mutex_table.lock);
  return 0;
}

int
mutex_release(int tag)
{
  int i;
  struct mutex *m;
  if(tag == TAG_UNALLOC){
    return -1;
  }
  acquire(&mutex_table.lock);
  for(i = 0; i < NMTX; ++i){
    if(mutex_table.mutexes[i].tag == tag)
      break;
  }
  //Tag not found
  if(i == NMTX){
    release(&mutex_table.lock);
    return -1;
  }

  //Release lock
  m = &(mutex_table.mutexes[i]);
  acquire(&m->lock);
  m->locked = 0;
  m->pid = 0;
  wakeup(m);
  release(&m->lock);
  release(&mutex_table.lock);

  return 0;
}


int
mutex_destroy(int tag)
{
  int i;
  if(tag == TAG_UNALLOC){
    return -1;
  }
  acquire(&mutex_table.lock);
  for(i = 0; i < NMTX; ++i){
    if(mutex_table.mutexes[i].tag == tag){
      mutex_table.mutexes[i].tag = TAG_UNALLOC;
      release(&mutex_table.lock);
      return 0;
    }
  }
  // Couldn't find the given tag in the mutex table
  release(&mutex_table.lock);
  return -1;
}
