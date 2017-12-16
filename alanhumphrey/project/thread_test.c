#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"

#define STDOUT      1
#define NUM_ITER    1
#define NUM_ELEMS   4
#define MAX_ELEMS  10



struct node {
   int data;
   struct node* next;
   struct node* prev;
};

typedef struct queue {
  struct node * head;
  struct node * tail;
} queue;

queue prod_con_q;
queue prod_con_interleaved_q;
int queue_lock;

volatile int run  = 1;
uint num_enqueued = 0;
uint num_dequeued = 0;


// ============================================================================
// print contents of queue, head-->tail
void print_queue(queue* q)
{
  struct node *n = q->head;
  printf(STDOUT, "[head] =>");
  while (n != 0) {
    printf(STDOUT, " %d =>", n->data);
    n = n->next;
  }
  printf(STDOUT, " [null]\n");
}

// ============================================================================
// initialize the specified queue
static void
queue_init(struct queue* q) {
  q->head = q->tail = 0;  // empty
}

// ============================================================================
// return 1 if head and tail both == NULL, 0 otherwise
static int
queue_empty(queue* q) {
  return ( (q->head == 0) && (q->tail == 0) );
}

// ============================================================================
// enqueue p into specified queue (append int at q->tail), adjust ptrs
static void
enqueue(queue* q, struct node* n) {

  mutex_acquire(queue_lock);
  {
    print_queue(&prod_con_q);

    // check for empty queue
    if (queue_empty(q)) {
      q->head = n;
      n->prev = n->next = 0;
    }

    // otherwise adjust ptrs
    else {
      q->tail->next = n;
      n->prev       = q->tail;
      n->next       = 0;
    }

    // for all cases, adjust tail ptr -> to newly added proc
    q->tail = n;

    num_enqueued++;

    printf(STDOUT, "TID: %d    enqueue: %d\n", thread_self(), (*n).data);
  }
  mutex_release(queue_lock);
}

// ============================================================================
// dequeue head element of specified queue, adjust ptrs
static struct node*
dequeue(queue* q) {

  struct node* n = 0;

  mutex_acquire(queue_lock);
  {
    // check for empty list
    if (queue_empty(q)) {
      return 0;
    }

    n = q->head;

    if (q->head->next) {
      q->head             = q->head->next;
      q->head->prev       = 0;
      n->prev = n->next   = 0;
    }

    // 1 element list
    else {
      queue_init(q); // reinit to be empty
    }
    num_dequeued++;
    printf(STDOUT, "TID: %d    dequeue: %d\n", thread_self(), (*n).data);
    free(n);
    print_queue(&prod_con_q);
  }
  mutex_release(queue_lock);

  return n;
}

void
producer_basic(void* arg)
{
  for (int i = 0; i < MAX_ELEMS; ++i) {
    struct node* n = malloc(sizeof(struct node));
    n->data = i;
    n->next = 0;
    n->prev = 0;
    enqueue(&prod_con_q, n);
  }
  printf(STDOUT, "total nodes enqueued: %d\n\n", __atomic_load_n(&num_enqueued, __ATOMIC_SEQ_CST));
}

void
producer_interleave(void* arg)
{
  while (__atomic_load_n(&run, __ATOMIC_SEQ_CST) == 1) {
    for (int i = 0; i < NUM_ELEMS; ++i) {
      struct node* n = malloc(sizeof(struct node));
      n->data = i;
      n->next = 0;
      n->prev = 0;
      enqueue(&prod_con_q, n);
    }
    thread_yield();
  }
  printf(STDOUT, "total nodes enqueued: %d\n\n", __atomic_load_n(&num_enqueued, __ATOMIC_SEQ_CST));
}

void
consumer(void* arg)
{
  while (run == 1) {
    if (queue_empty(&prod_con_q)) {
      printf(STDOUT, "TID: %d - empty queue\n", thread_self());
      return;
    } else {
      dequeue(&prod_con_q);
      thread_yield();
    }
  }
}

void
start_routine_1(void* arg) {
	for(int i = 0; i < (uint)arg; i++) {
	  printf(STDOUT, "TID: %d iteration %d in start_routine_1, arg: %d\n", thread_self(), i, arg);
	}
	printf(STDOUT, "\n");
}

void
start_routine_2(void* arg) {
	for(int i = 0; i < (int)arg; i++) {
		printf(STDOUT, "TID: %d iteration %d in start_routine_2, arg: %d\n", thread_self(), i, arg);
	}
}

void
start_routine_3(void* arg) {
	for(int i = 0; i < (int)arg; i++) {
	  printf(STDOUT, "TID: %d iteration %d in start_routine_3, arg: %d \n", thread_self(), i, arg);
	}
}

void
start_routine_4(void* arg) {
  for(int i = 0; i < (int)arg; i++) {
    printf(STDOUT, "TID: %d iteration %d in start_routine_4, arg: %d \n", thread_self(), i, arg);
  }
}

void
start_routine_5(void* arg) {
  for(int i = 0; i < (int)arg; i++) {
    printf(STDOUT, "TID: %d iteration %d in start_routine_5, arg: %d \n", thread_self(), i, arg);
  }
}

void
start_routine_6(void* arg) {
  for(int i = 0; i < (int)arg; i++) {
    printf(STDOUT, "TID: %d iteration %d in start_routine_6, arg: %d \n", thread_self(), i, arg);
  }
}

void
single_proc_test()
{
  printf(STDOUT, "\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Running single proc test....\n");
  printf(STDOUT, "-------------------------------------------------\n");

  thread_init();

  int tid1 = thread_create(start_routine_1, (void*)1);
  int tid2 = thread_create(start_routine_2, (void*)2);
  int tid3 = thread_create(start_routine_3, (void*)3);
  int tid4 = thread_create(start_routine_4, (void*)4);
  int tid5 = thread_create(start_routine_5, (void*)5);
  int tid6 = thread_create(start_routine_6, (void*)6);

  thread_join(tid1);
  thread_join(tid2);
  thread_join(tid3);
  thread_join(tid4);
  thread_join(tid5);
  thread_join(tid6);

  printf(STDOUT, "TID: %d done with single_proc_test()\n", thread_self());

  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Finished single proc test.\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "\n");

}

void
multi_proc_test()
{
  printf(STDOUT, "\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Running multi proc test....\n");
  printf(STDOUT, "-------------------------------------------------\n");

  thread_init();

  for (int i = 0; i < NUM_ITER; ++i) {

    // child proc
    if (fork() == 0) {
      int tid1 = thread_create(start_routine_1, (void*)1);
      int tid2 = thread_create(start_routine_2, (void*)2);
      int tid3 = thread_create(start_routine_3, (void*)3);
      int tid4 = thread_create(start_routine_4, (void*)4);
      int tid5 = thread_create(start_routine_5, (void*)5);
      int tid6 = thread_create(start_routine_6, (void*)6);

      thread_join(tid1);
      thread_join(tid2);
      thread_join(tid3);
      thread_join(tid4);
      thread_join(tid5);
      thread_join(tid6);

      printf(STDOUT, "TID: %d child proc done with multi_proc_test()\n", thread_self());
    }
    // parent
    else {
      int tid1 = thread_create(start_routine_1, (void*)7);
      int tid2 = thread_create(start_routine_2, (void*)8);
      int tid3 = thread_create(start_routine_3, (void*)9);
      int tid4 = thread_create(start_routine_4, (void*)10);
      int tid5 = thread_create(start_routine_5, (void*)11);
      int tid6 = thread_create(start_routine_6, (void*)12);

      thread_join(tid1);
      thread_join(tid2);
      thread_join(tid3);
      thread_join(tid4);
      thread_join(tid5);
      thread_join(tid6);

      printf(STDOUT, "TID: %d parent proc done with multi_proc_test()\n", thread_self());

      wait();
    }
  }

  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Finished multi proc test.\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "\n");
}

void
prod_con_test(void)
{
  printf(STDOUT, "\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Running producer-consumer test....\n");
  printf(STDOUT, "-------------------------------------------------\n");

  thread_init();

  queue_lock  = mutex_create();

  int producer_t1 = thread_create(producer_basic, (void*)0);
  run = 1;

  int consumer_t1 = thread_create(consumer, (void*)0);
  int consumer_t2 = thread_create(consumer, (void*)0);
  int consumer_t3 = thread_create(consumer, (void*)0);
  int consumer_t4 = thread_create(consumer, (void*)0);
  int consumer_t5 = thread_create(consumer, (void*)0);

  thread_join(producer_t1);
  thread_join(consumer_t1);
  thread_join(consumer_t2);
  thread_join(consumer_t3);
  thread_join(consumer_t4);
  thread_join(consumer_t5);

  mutex_destroy(queue_lock);

  printf(STDOUT, "TID: %d done with prod_con_test()\n", thread_self());

  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "        Finished producer-consumer test.\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "\n");

}

void
prod_con_interleaved_test(void)
{
  printf(STDOUT, "\n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "   Running interleaved producer-consumer test....\n");
  printf(STDOUT, "-------------------------------------------------\n");

  thread_init();

  queue_init(&prod_con_q);

  queue_lock  = mutex_create();

  int producer_t1 = thread_create(producer_interleave, (void*)&prod_con_q);
  run = 1;

  int consumer_t1 = thread_create(consumer, (void*)&prod_con_q);
  int consumer_t2 = thread_create(consumer, (void*)&prod_con_q);
  int consumer_t3 = thread_create(consumer, (void*)&prod_con_q);
  int consumer_t4 = thread_create(consumer, (void*)&prod_con_q);
  int consumer_t5 = thread_create(consumer, (void*)&prod_con_q);

  thread_join(producer_t1);
  thread_join(consumer_t1);
  thread_join(consumer_t2);
  thread_join(consumer_t3);
  thread_join(consumer_t4);
  thread_join(consumer_t5);

  mutex_destroy(queue_lock);

  printf(STDOUT, "TID: %d done with prod_con_test()\n", thread_self());

  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "   Finished interleaved producer-consumer test.  \n");
  printf(STDOUT, "-------------------------------------------------\n");
  printf(STDOUT, "\n");

}

int
main(int argc, char **argv) {

	single_proc_test();

	sleep(100);

	prod_con_test();

	sleep(100);

  multi_proc_test();

  sleep(100);

  prod_con_interleaved_test();

	exit();
}
