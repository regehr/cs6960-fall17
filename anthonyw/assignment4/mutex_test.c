#include "types.h"
#include "user.h"
#include "syscall.h"

#define MTAG 256
#define PLIM 3

//This isn't much of a test at all, but due to other homework I didn't have
//as much time to actually test.
//If the sleeplock implementation is correct (since I based my implementation on
//that), the mutex should work.
int main(int argc, char **argv)
{
  int i;
  mutex_create(MTAG);
  for(i = 1; i <= PLIM; i++){
    if(fork() == 0){
      mutex_acquire(MTAG);
      printf(1, "child acquired lock\n");
      mutex_release(MTAG);
      printf(1, "child released lock\n");
      exit();
    }
  }
  mutex_destroy(MTAG);
  exit();
}
