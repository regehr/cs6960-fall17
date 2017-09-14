
#include "types.h"
#include "defs.h"
#include "spinlock.h"


#define MAX_MUTEXES 16


// =================================================================================================
// The mutex type itself, using existing xv6 spinlock.
struct mutex {

  int    acquired;
  struct spinlock lock;

};


// =================================================================================================
// The list of mutexes with access protected by spinlock.
//   Naming scheme will be integer based, easy with a static list of mutexes.
struct {

  struct mutex list[MAX_MUTEXES];  // the list of mutexes (static number of elements for now)
  char   states[MAX_MUTEXES];        // states:   0-released,   1-acquired
  struct spinlock lock;              // protect access to the list of mutexes

} mutex_list;


// =================================================================================================
// Handle some basic error checking
//
static int
mutex_error_check(int mtx_id) {

  if ( (mtx_id < 0) || (mtx_id > MAX_MUTEXES) ) {
    return -1;
  }

  return 0;
}


// =================================================================================================
// This is called from main() in main.c
//   Initialize our list of mutexes
void
mutex_init(void) {

  initlock(&mutex_list.lock, "mutex_list");

  // Initialize each spinlock (per spinlock API), also acquired field and state
  for (int i = 0; i < MAX_MUTEXES; ++i) {
    initlock(&mutex_list.list[i].lock, "mutex");
    mutex_list.list[i].acquired = 0;             // not initially acquired
    mutex_list.states[i]        = 0;             // initially available
  }

  // full memory barrier before proceeding (probably don't need this)
  __sync_synchronize();
}


// =================================================================================================
// Create a mutex
//   Return 0 on success and -1 if the associated mutex does not exist or could not be created.
int
mutex_create(void) {

  acquire(&mutex_list.lock);
  {
    for (int i = 0; i < MAX_MUTEXES; ++i) {
      if (!mutex_list.states[i]) {
        mutex_list.states[i] = 1;
        release(&mutex_list.lock);
        return i;
      }
    }
  }
  release(&mutex_list.lock);

  return -1;
}


// =================================================================================================
// Destroy the mutex with the specified id.
//   Return 0 on success and -1 if the associated mutex does not exist or could not be destroyed.
int
mutex_destroy(int mtx_id) {

  int err = 0;
  if ( (err = mutex_error_check(mtx_id)) != 0 ) {
    return -1;
  }

  acquire(&mutex_list.lock);
  {
    // check if destroying an unused mutex
    if (mutex_list.states[mtx_id] == 0) {
      release(&mutex_list.lock);
      return -1;
    }
    mutex_list.states[mtx_id] = 0;
  }
  release(&mutex_list.lock);

  return 0;
}


// =================================================================================================
// Acquire the mutex with the specified id.
//   Return 0 on success and -1 if the associated mutex does not exist or is invalid.
int
mutex_acquire(int mtx_id) {

  int err = 0;
  if ( (err = mutex_error_check(mtx_id)) != 0 ) {
    return -1;
  }

  struct mutex* mtx = &mutex_list.list[mtx_id];

  acquire(&mtx->lock);
  {
    while (mtx->acquired) {
      sleep(mtx, &mtx->lock);
    }
    mtx->acquired = 1;
  }
  release(&mtx->lock);

  return 0;
}


// =================================================================================================
// Release the mutex with the specified id.
//   Return 0 on success and -1 if the associated mutex does not exist or is invalid.
int
mutex_release(int mtx_id) {

  int err = 0;
  if ( (err = mutex_error_check(mtx_id)) != 0 ) {
    return -1;
  }

  struct mutex* mtx = &mutex_list.list[mtx_id];

  acquire(&mtx->lock);
  {
    // check if releasing an unacquired mutex
    if (mtx->acquired == 0) {
      release(&mtx->lock);
      return -1;
    }

    mtx->acquired = 0;
    wakeup(mtx);
  }
  release(&mtx->lock);

  return 0;
}



