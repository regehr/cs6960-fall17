#include "types.h"
#include "defs.h"
#include "spinlock.h"

#define NMUTEX 64

struct mutex {
  int hold;
  struct spinlock lk;
};

enum mtx_state {
  USED, UNUSED
};

struct {
  struct spinlock lk;
  enum mtx_state states[NMUTEX];
  struct mutex mtxs[NMUTEX];
} mtxtable;

void mtx_table_init(void)
{
  initlock(&mtxtable.lk, "mtxtable");

  acquire(&mtxtable.lk);
  int i;
  for (i = 0; i < NMUTEX; ++i) {
    mtxtable.states[i] = UNUSED;
    initlock(&(mtxtable.mtxs[i].lk), "mtx");
    mtxtable.mtxs[i].hold = 0;
  }
  release(&mtxtable.lk);
}


int mtx_release(int mtx_id) {
  if (mtx_id < 0 || mtx_id >= NMUTEX || 
      mtxtable.states[mtx_id] == UNUSED)
    return -1;

  struct mutex* mtx = &(mtxtable.mtxs[mtx_id]);
  acquire(&mtx->lk);
  mtx->hold = 0;
  wakeup(mtx);
  release(&mtx->lk);
  return 0;
}

int mtx_acquire(int mtx_id)
{
  if (mtx_id < 0 || mtx_id >= NMUTEX || 
      mtxtable.states[mtx_id] == UNUSED)
    return -1;

  struct mutex* mtx = &(mtxtable.mtxs[mtx_id]);
  acquire(&mtx->lk);
  while (mtx->hold) {
    sleep(mtx, &mtx->lk);
  }
  mtx->hold = 1;
  release(&mtx->lk);
  return 0;
}


int mtx_create(void)
{
  acquire(&mtxtable.lk);

  int i;
  for (i = 0; i < NMUTEX; ++i) {
    if (mtxtable.states[i] == UNUSED) {
      mtxtable.states[i] = USED;
      release(&mtxtable.lk);
      return i;
    }
  }

  release(&mtxtable.lk);
  return -1;
}

int mtx_destroy(int mtx_id) {
  acquire(&mtxtable.lk);
  if (mtx_id < 0 || mtx_id >= NMUTEX ||
      mtxtable.states[mtx_id] == UNUSED) {
    release(&mtxtable.lk);
    return -1;
  }
  mtxtable.states[mtx_id] = UNUSED;    
  release(&mtxtable.lk);
  return 0;
}




