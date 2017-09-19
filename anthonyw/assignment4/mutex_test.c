#include "types.h"
#include "user.h"
#include "syscall.h"

#define MTAG 256

//This isn't much of a test at all, but due to other homework I didn't have
//as much time to actually test.
//If the sleeplock implementation is correct (since I based my implementation on
//that), the mutex should work.
int main(int argc, char **argv)
{
  mutex_create(MTAG);
  if(fork() == 0){
    sleep(2);
    mutex_acquire(MTAG);
    printf(1, "child acquired lock\n");
    mutex_release(MTAG);
    printf(1, "child released lock\n");
    exit();
  }
  mutex_acquire(MTAG);
  printf(1, "parent acquired lock\n");
  mutex_release(MTAG);
  printf(1, "parent released lock\n");

  wait();
  mutex_destroy(MTAG);
  exit();
}
