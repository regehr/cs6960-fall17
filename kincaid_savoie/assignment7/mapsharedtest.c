/**
 * Test that the shared memory mapping works correctly.
 */

#include "types.h"
#include "user.h"

typedef struct test {
  int number;
  char letter;
} test;

int
main(void)
{
  int pid = fork();

  if (pid == 0) {
    // Map a shared page
    char *data = mapshared();
    if (!data) {
      printf(2, "Could not map shared page!\n");
      exit();
    }

    // Attempt to map the page again
    char *new_data = mapshared();
    if (new_data) {
      printf(2, "Was able to map shared memory twice!\n");
      exit();
    }

    // Store some kind of struct in the page
    test in_struct;
    in_struct.number = 5;
    in_struct.letter = 'd';

    memmove(data, &in_struct, sizeof(test));
  } else {
    // Make sure we go after the parent.
    sleep(500);

    // Map a shared page
    char *data = mapshared();
    if (!data) {
      printf(2, "Could not map shared page in child!\n");
      exit();
    }

    // Attempt to map that page again
    char *new_data = mapshared();
    if (new_data) {
      printf(2, "Was able to map shared memory twice in child!\n");
      exit();
    }
    
    // Attempt to pull the struct out of the page
    test out_struct;

    memmove(&out_struct, data, sizeof(test));

    if (out_struct.number != 5 || out_struct.letter != 'd') {
      printf(2, "Data was corrupted during transfer!\n");
      exit();
    }

    exit();
  }

  wait();
  exit();
}
