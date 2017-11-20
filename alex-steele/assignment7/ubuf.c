
#include "types.h"
#include "user.h"

#define PAGESZ 4096
#define RINGSZ ((8*PAGESZ)-(2*sizeof(int)))

static int min(int a, int b) 
{
  return a < b ? a : b;
}

static int load(int *ptr)
{
  return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static void store(int *ptr, int val)
{
  __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

static struct {
  int roff;
  int woff;
  char data[RINGSZ];
} *ring;

int buf_setup(void)
{
  if (ring)
    return -1;
  ring = mshared();
  if (!ring)
    return -1;
  return 0;
}

int buf_put(char *data, int len)
{
  int roff = load(&ring->roff);
  int woff = (load(&ring->woff) + 3) & ~0x03;
  int wsize;

  if (woff - roff + len > RINGSZ) {
    return -1;
  }

  // Record the message length
  *((int *) &ring->data[woff % RINGSZ]) = len;
  woff += 4;

  // Record the message
  wsize = min(len, RINGSZ - (woff % RINGSZ));
  memmove(&ring->data[woff % RINGSZ], data, wsize);
  woff += wsize;
  memmove(&ring->data[woff % RINGSZ], data + wsize, len - wsize);
  woff += len - wsize; 

  store(&ring->woff, woff);

  return 0;
}

int buf_get(char *data, int *len, int maxlen)
{
  int roff = (load(&ring->roff) + 3) & ~0x03;
  int woff = load(&ring->woff);
  int mlen;
  int rsize;

  if (roff >= woff) {
    return -1;
  }

  // Read the message length
  mlen = *((int *) &ring->data[roff % RINGSZ]);

  if (mlen > maxlen) {
    return -2;
  }
  roff += 4;

  // Read the message
  rsize = min(mlen, RINGSZ - (roff % RINGSZ));
  memmove(data, &ring->data[roff % RINGSZ], rsize);
  roff += rsize;
  memmove(data + rsize, &ring->data[roff % RINGSZ], mlen - rsize);
  roff += mlen - rsize;
  *len = mlen;

  store(&ring->roff, roff);

  return 0;
}

