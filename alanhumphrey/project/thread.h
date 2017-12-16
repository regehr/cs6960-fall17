
#define STACK_SIZE    4096
#define MAX_THREADS      7


typedef enum thread_state {
    FREE
  , RUNNING
  , RUNNABLE
  , SLEEP
} thread_state;


struct thread {
  uint             tid;     // thread id
  uint             esp;     // stack pointer register
  uint             ebp;     // stack base pointer register
  char          * stack;    // thread stack
  thread_state    state;    // thread state
};


int  thread_init();

int  thread_create(void (*start_routine)(void *), void* arg);

int  thread_join(int tid);

void thread_yield();

void thread_exit();

int  thread_self();

