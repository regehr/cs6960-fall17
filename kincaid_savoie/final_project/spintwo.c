#include "types.h"
#include "user.h"

int
main(void)
{
  fork();

  uint counter = 0;
  while (1) {
    counter++;
    if (counter % 10 == 0)
      printf(1, "%d\n", counter);
  }
  exit();
}
