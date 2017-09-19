#include "spinlock.h"
#include "defs.h"

// sysmutex implementation

struct sysmutex
{
  int locked;
  int in_use;
};

struct mutextable
{
  struct sysmutex mutexes[4];
  struct spinlock mutexes_lock;
} mutex_table;

void
mutexinit(void)
{
  initlock(&mutex_table.mutexes_lock, "mutexes_lock"); 
  for (int m = 0; m < 4; ++m)
  {
    mutex_table.mutexes[m].locked = 0;
  }
}

int
mutexcreate(void)
{
  acquire(&mutex_table.mutexes_lock);
  for (int m = 0; m < 4; m++)
  {
    if (!mutex_table.mutexes[m].in_use)
    {
      mutex_table.mutexes[m].in_use = 1;
      mutex_table.mutexes[m].locked = 0;
      release(&mutex_table.mutexes_lock);
      return m;
    }
  }
  // Out of mutexes, womp womp :(
  release(&mutex_table.mutexes_lock);
  return -1;
}

int 
mutexdestroy(int mutex)
{
  acquire(&mutex_table.mutexes_lock);
  // Make sure mutex exists.
  if (mutex > 4)
  {
    release(&mutex_table.mutexes_lock);
    return -1;
  }
  mutex_table.mutexes[mutex].locked = 0; 
  mutex_table.mutexes[mutex].in_use = 0;
  return 0;
}

int
mutexlock(int mutex)
{
  acquire(&mutex_table.mutexes_lock);
  // Make sure mutex exists.
  if (mutex > 4 || !mutex_table.mutexes[mutex].in_use)
  {
    release(&mutex_table.mutexes_lock);
    return -1;
  }
  while(mutex_table.mutexes[mutex].locked)
  {
    sleep(&mutex_table, &mutex_table.mutexes_lock);
  }
  mutex_table.mutexes[mutex].locked = 1;
  return 0;
}

int
mutexunlock(int mutex)
{
  acquire(&mutex_table.mutexes_lock);
  // Make sure mutex exists.
  if (mutex > 4)
  {
    release(&mutex_table.mutexes_lock);
    return -1;
  }
  while(mutex_table.mutexes[mutex].locked)
  {
    sleep(&mutex_table, &mutex_table.mutexes_lock);
  }
  mutex_table.mutexes[mutex].locked = 0;
  return 0;
}
