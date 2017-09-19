#include "types.h"
#include "user.h"

int
main(void) {
  printf(1, "Creating a mutex...\n");

  int mutex = mutex_create();

  if (mutex < 0) {
    printf(1, "Mutex creation failed.\n");
    exit();
  }

  printf(1, "Testing mutex...\n");

  int pid = fork();

  int sum = 0;

  int i;
  for (i = 0; i < 10000; i++) {
    mutex_acquire(mutex);

    int new_sum = sum + 100;

    int j;
    for (j = 0; j < 100; j++) {
      sum++;
    }

    if (sum != new_sum) {
      printf(1, "Incorrect sum obtained!\n");
      exit();
    }

    mutex_release(mutex);
  }

  if (pid != 0) {
    wait();
  
    printf(1, "Test successfull! Destroying mutex...\n");

    mutex_destroy(mutex);

    printf(1, "All done!\n");
  }

  exit();
}
