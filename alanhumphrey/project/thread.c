#include "types.h"
#include "user.h"
#include "x86.h"
#include "thread.h"

struct  thread   thread_pool[MAX_THREADS];  // array of threads
volatile int     next_tid;                  // next tid to run
volatile int     curr_tid_idx;              // index of currently running thread


static
void run_thread( void (*start_routine)( void * ), void* arg )
{
  start_routine(arg);
  printf(1, "TID: %d done with start_routine()\n", thread_self());
  thread_exit();
}


int
thread_init()
{
  printf(1, "thread_init() called....\n");

  for (int i = 0; i < MAX_THREADS; ++i) {
    thread_pool[i].state = FREE;
  }

  curr_tid_idx = 0;
  thread_pool[curr_tid_idx].tid   = 0;
  thread_pool[curr_tid_idx].stack = 0;
  thread_pool[curr_tid_idx].state = RUNNING;
  next_tid = 1;

  return 0;
}


int
thread_self( void )
{
  // return tid of current running thread
  return thread_pool[curr_tid_idx].tid;
}


int
thread_create( void (*start_routine)( void * ), void* arg )
{
  if (__atomic_load_n(&next_tid, __ATOMIC_SEQ_CST) > MAX_THREADS) {
    printf(1, "Exceeded MAX_THREADS... (%d > %d)", __atomic_load_n(&next_tid, __ATOMIC_SEQ_CST), MAX_THREADS);
    return -1;
  }

  for (int i = 0; i < MAX_THREADS; ++i) {
    if (thread_pool[i].state == FREE) {
      thread_pool[i].tid = next_tid;  // assign a thread ID
      printf(1, "TID: %d created new thread (new tid: %d)\n", thread_self(), next_tid);
      next_tid++;
      thread_pool[i].stack = malloc(STACK_SIZE);

      // load thread execute function to stack with arguments and return value
      *((int*)(thread_pool[i].stack + STACK_SIZE - 3 * sizeof(int))) = 0;
      *((int*)(thread_pool[i].stack + STACK_SIZE - 2 * sizeof(int))) = (int)start_routine;
      *((int*)(thread_pool[i].stack + STACK_SIZE - sizeof(int))) = (int)arg;

      // set thread  so it's ready to run
      thread_pool[i].state = RUNNABLE;

      // set esp and ebp to null, while this thread is not in RUNNING state
      thread_pool[i].esp = 0;
      thread_pool[i].ebp = 0;

      return thread_pool[i].tid;
    }
  }
  return -1;
}


void
thread_yield( void )
{
  static int next_tid_idx = 0;
  next_tid_idx = curr_tid_idx+1;

  // if current thread is running, the thread didn't finish its job so turn its state to RUNNABLE
  if (thread_pool[curr_tid_idx].state == RUNNING) {
    thread_pool[curr_tid_idx].state = RUNNABLE;
  }

  //----------------------------------------------------------------------------
  //                     CONTEXT SWITCH
  //----------------------------------------------------------------------------

  // save current thread esp and ebp
  asm("movl %%esp, %0;" : "=r" (thread_pool[curr_tid_idx].esp));
  asm("movl %%ebp, %0;" : "=r" (thread_pool[curr_tid_idx].ebp));

  // round robin schedule
  if (next_tid_idx == MAX_THREADS) {
    next_tid_idx = 0;
  }

  for (int j = next_tid_idx; j < MAX_THREADS; ++j) {
    next_tid_idx++;
    if (thread_pool[j].state == RUNNABLE) {
        curr_tid_idx = j;
        break;
    }
  }

  // current switched thread is move to RUNNING mode, next to be execute
  thread_pool[curr_tid_idx].state = RUNNING;

  //----------------------------------------------------------------------------
  //                     CONTEXT SWITCH
  //----------------------------------------------------------------------------

  // thread is running for the first time, load and jump to run_thread function
  if (thread_pool[curr_tid_idx].esp == 0) {
    asm("movl %0, %%esp;" : : "r" (thread_pool[curr_tid_idx].stack + STACK_SIZE - 3*sizeof(int)));
    asm("movl %0, %%ebp;" : : "r" (thread_pool[curr_tid_idx].stack + STACK_SIZE - 3*sizeof(int)));
    asm("jmp *%0;" : : "r" (run_thread));
  }
  else {
    // restore thread stack
    asm("movl %0, %%esp;" : : "r" (thread_pool[curr_tid_idx].esp));
    asm("movl %0, %%ebp;" : : "r" (thread_pool[curr_tid_idx].ebp));
  }
}

int
thread_join( int tid )
{
  printf(1, "TID: %d calling thread_join() for tid: %d\n", thread_self(), tid);

  if (tid >= next_tid || tid < 0) {
    return -1;
  }

  for (int i = 0; i < MAX_THREADS; ++i) {
    if (thread_pool[i].tid == tid) {
      // put current running thread to sleep
      printf(1, "TID: %d going to sleep...\n\n", thread_self());
      thread_pool[curr_tid_idx].state = SLEEP;
      // yield so another thread can do its thing
      thread_yield();
      break;
    }
  }
  return 0;
}


void
thread_exit( void )
{
  printf(1, "TID: %d called thread_exit()\n\n", thread_self());

  // cleanup thread stack
  if (thread_pool[curr_tid_idx].stack != 0) {
    free(thread_pool[curr_tid_idx].stack);
  }

  // deallocate thread from thread pool
  thread_pool[curr_tid_idx].state = FREE;

  // if any thread is waiting on current thread, thenmake it RUNNABLE
  for (int i = 0; i < MAX_THREADS; ++i) {
    if (thread_pool[i].state == SLEEP) {
      thread_pool[i].state = RUNNABLE;
    }
  }

  // check for free threads
  for (int j = 0; j < MAX_THREADS; ++j) {
    if (thread_pool[j].state != FREE) {
      // found thread that is eligible to run, yield
      thread_yield();
    }
  }

  // bail if no thread can run
  exit();
}
