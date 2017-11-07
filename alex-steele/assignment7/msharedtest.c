
#include "types.h"
#include "user.h"

int main(void)
{
  char *data;

  printf(1, "mshared() test\n");

  data = mshared();
  if (!data) {
    printf(2, "mshared() failed\n");
    exit();
  }

  if (mshared()) {
    printf(2, "error: mshared() successfully called twice\n");
    exit();
  }

  if (fork() == 0) {
    if (mshared()) {
      printf(2, "error: child able to call mshared()\n");
      exit();
    }
    sleep(100);
    printf(1, "%s\n", data);
    strcpy(data+4096, "hello from child");
    exit();
  }

  strcpy(data, "hello from parent");
  sleep(200);
  printf(1, "%s\n", data+4096);
  wait();
  exit();
}
  

