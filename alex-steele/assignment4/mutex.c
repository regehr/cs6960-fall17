
//
// mutex.c - Alex Steele
//
// General allocation approach borrowed from proc.c.
//

#include "types.h"
#include "defs.h"
#include "spinlock.h"

#define NMUTEX 64

enum mutex_state {
  UNUSED,
  USED
};

struct mutex {
  struct spinlock lock;
  enum mutex_state state;
  int held;
};

struct {
  struct spinlock lock;
  struct mutex mutexes[NMUTEX];
} mtable;

static void
initm(struct mutex *m)
{
  initlock(&m->lock, "mutex");
  m->state = UNUSED;
  m->held = 0;
}

void
mtableinit(void)
{
  struct mutex *m;
  
  initlock(&mtable.lock, "mtable");
  
  for (m = mtable.mutexes; m < &mtable.mutexes[NMUTEX]; m++)
    initm(m);
}

static void
lockm(struct mutex *m)
{
  acquire(&m->lock);
  while (m->held) {
    sleep(m, &m->lock);    
  }
  m->held = 1;
  release(&m->lock);
}

static void
unlockm(struct mutex *m)
{
  acquire(&m->lock);
  m->held = 0;
  wakeup(m);
  release(&m->lock);
}

int
mutexcreate(void)
{
  int i;
  
  acquire(&mtable.lock);
  for (i = 0; i < NMUTEX; i++) {
    if (mtable.mutexes[i].state == UNUSED) {
      mtable.mutexes[i].state = USED;
      release(&mtable.lock);
      return i;
    }
  }
  release(&mtable.lock);
  return -1;
}

static struct mutex *
getmutex(int mid)
{
  if (mid < 0 || mid >= NMUTEX)
    return 0;
  if (mtable.mutexes[mid].state == UNUSED)
    return 0;
  return mtable.mutexes + mid;
}

int
mutexlock(int mid)
{
  struct mutex *m = getmutex(mid);

  if (!m)
    return -1;

  lockm(m);
  return 0;
}

int
mutexunlock(int mid)
{
  struct mutex *m = getmutex(mid);

  if (!m)
    return -1;

  unlockm(m);
  return 0;
}

int
mutexdestroy(int mid)
{
  struct mutex *m = getmutex(mid);

  if (!m)
    return -1;

  acquire(&mtable.lock);
  if (m->state == UNUSED) {
    release(&mtable.lock);
    return -1;
  }
  m->state = UNUSED;
  release(&mtable.lock);
  return 0;
}
