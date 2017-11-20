#include "types.h"
#include "user.h"

int main()
{
  int pid;
  char *ringbuf = (char *)0;

  pid = fork();
  if(pid == 0){
    ringbuf = memshared();
    if(ringbuf != 0){
      *ringbuf = 'a';
      printf(1, "Child: wrote '%c' to shared memory\n", *ringbuf);
    }
    exit();
  } else if(pid > 0){
    ringbuf = memshared();
    wait();
    while(*ringbuf != 'a'){
      /* busy wait */
      printf(1, "first char is %d\n", *ringbuf);
      sleep(2);
    }
    printf(1, "Parent: got 'a' from child process!\n");
    wait();
    printf(1, "Second call to memshared should give 0. Result: %d\n", memshared());
  } else {
    printf(1, "fork() failed. exiting...\n");
  }

  exit();
  return 0;
}
