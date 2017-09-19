#include "types.h"
#include "defs.h"
#include "spinlock.h"

#define NUM_MUTEX 10

struct mutex {
  int acquired;
  int in_use;
};

struct {
  struct mutex mutexes[NUM_MUTEX];
  struct spinlock lock;
} mtable;

void
init_mutex_table(void) {
  initlock(&mtable.lock, "mtable");

  // Set all mutexes to available
  int i;
  for (i = 0; i < NUM_MUTEX; i++) {
    mtable.mutexes[i].acquired = 0;
    mtable.mutexes[i].in_use = 0;
  }
}

int
mutex_create(int *mutex_id) {
  acquire(&mtable.lock);

  int i;
  for (i = 0; i < NUM_MUTEX; i++) {
    if (mtable.mutexes[i].in_use == 0) {
      mtable.mutexes[i].in_use = 1;
      *mutex_id = i;
      release(&mtable.lock);
      return 0;
    }
  }

  // There were no available mutexes, return.
  release(&mtable.lock);
  return -1;
}

int
mutex_acquire(int mutex_id) {
  // Make sure the mutex id makes sense.
  if (mutex_id < 0 || mutex_id >= NUM_MUTEX) {
    return -1;
  }

  struct mutex *acquire_mutex = &mtable.mutexes[mutex_id];

  // Make sure the specified mutex is actually in-use.
  if (acquire_mutex->in_use == 0) {
    return -1;
  }

  // Wait for the mutex to show as available.
  acquire(&mtable.lock);

  while (acquire_mutex->acquired) {
    sleep(acquire_mutex, &mtable.lock);
  }

  acquire_mutex->acquired = 1;

  release(&mtable.lock);

  return 0;
}

int
mutex_release(int mutex_id) {
  // Make sure the mutex id makes sense.
  if (mutex_id < 0 || mutex_id >= NUM_MUTEX) {
    return -1;
  }

  struct mutex *release_mutex = &mtable.mutexes[mutex_id];

  acquire(&mtable.lock);

  if (release_mutex->in_use == 0 || release_mutex->acquired == 0) {
    release(&mtable.lock);
    return -1;
  }

  release_mutex->acquired = 0;
  wakeup(release_mutex);

  release(&mtable.lock);

  return 0;
}

int
mutex_destroy(int mutex_id) {
  // Make sure the mutex id makes sense.
  if (mutex_id < 0 || mutex_id >= NUM_MUTEX) {
    return -1;
  }

  struct mutex *destroy_mutex = &mtable.mutexes[mutex_id];

  acquire(&mtable.lock);

  if (destroy_mutex->in_use == 0 || destroy_mutex->acquired == 1) {
    release(&mtable.lock);
    return -1;
  }

  destroy_mutex->in_use = 0;

  release(&mtable.lock);

  return 0;
}
