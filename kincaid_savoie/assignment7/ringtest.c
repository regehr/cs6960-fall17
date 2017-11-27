/**
 * Test the implementation of the shared buffer.
 */

#include "types.h"
#include "user.h"

typedef struct test {
  int some_int;
  char some_char;
} test;

int
main(void)
{
  if (buf_setup() < 0) {
    printf(2, "Buffer setup failed!\n");
    exit();
  }

  // Try to set up the buffer again.
  if (buf_setup() == 0) {
    printf(2, "Buffer setup worked twice!\n");
    exit();
  }

  int pid = fork();

  if (pid != 0) {
    test test_data;
    test_data.some_int = 5;
    test_data.some_char = 'c';

    if (buf_put((void *) &test_data, sizeof(test_data)) < 0) {
      printf(2, "Buffer put failed!\n");
      exit();
    }
  } else {
    sleep(100);

    test test_data;

    uint test_len;
    if (buf_get((void *) &test_data, &test_len, sizeof(test_data)) < 0) {
      printf(2, "Buffer get failed!\n");
      exit();
    }

    if (test_data.some_int != 5 || test_data.some_char != 'c') {
      printf(2, "Incorrect data in test struct!\n");
      exit();
    }
    exit();
  }

  wait();
  printf(1, "Test successful!\n");
  exit();
}

