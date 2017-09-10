
#include "types.h"
#include "user.h"

int main(int argc, char **argv)
{
  int i;

  printf(1, "exit test 2\n");
  printf(1, "argc: %d\n", argc);
  for (i = 0; i < argc; i++) {
    printf(1, "%s\n", argv[i]);
  }
  return 0;
}
