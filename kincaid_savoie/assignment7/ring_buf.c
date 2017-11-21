#include "types.h"
#include "user.h"

#define PAGE_SIZE 4096
#define BUF_HEADER_SIZE (2 * sizeof(int))
#define BUF_SIZE (PAGE_SIZE * 8 - BUF_HEADER_SIZE)


int
buf_setup(void)
{
  return -1;
}

int 
buf_put(char *data, int len)
{
  return -1;
}

int
buf_get(char *data, int *len, int maxlen)
{
  return -1;
}

