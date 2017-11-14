#include "types.h"
#include "user.h"
#include "memlayout.h"

#define UBUFSIZE ((SMSIZE * 4096) - (2 * sizeof(int)))


static struct {
  int nread;
  int nwrite;
  char data[UBUFSIZE];
} *userbuf;


static int atomic_load(int* x) {
  return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

static void atomic_store(int* x, int val) {
  return __atomic_store_n(x, val, __ATOMIC_SEQ_CST);
}


int buf_init(void) {
  if (userbuf) return -1;
  userbuf = init_sm();
  if (!userbuf) return -1;
  return 0;
}

int buf_write(char* data, int len) {
  int nread = atomic_load(&(userbuf->nread));
  int nwrite = (atomic_load(&(userbuf->nwrite)) + 3) & ~0x03;
  
  if (UBUFSIZE - nwrite + nread < len + sizeof(int)) return -1;
  
  memmove(&userbuf->data[nwrite % UBUFSIZE], &len, sizeof(int));
  nwrite += sizeof(int);
  
  int bytes_to_write = (len < UBUFSIZE - (nwrite % UBUFSIZE))? len : (UBUFSIZE - (nwrite % UBUFSIZE));
  memmove(&userbuf->data[nwrite % UBUFSIZE], data, bytes_to_write);
  nwrite += bytes_to_write;
  memmove(&userbuf->data[nwrite % UBUFSIZE], data + bytes_to_write, len - bytes_to_write);
  nwrite += len - bytes_to_write;
  
  atomic_store(&userbuf->nwrite, nwrite);
  return 0;
}
int buf_read(char *data, int *len, int maxlen) {
  int nread = (atomic_load(&(userbuf->nread)) + 3) & ~0x03;
  int nwrite = atomic_load(&(userbuf->nwrite));
  
  if (nwrite == nread) return -1;
  int tmp_len;
  memmove(&tmp_len, &userbuf->data[nread % UBUFSIZE], sizeof(int));
  if (tmp_len > maxlen) return -2;
  nread += sizseof(int);
  
  int bytes_to_read = (tmp_len < UBUFSIZE - (nread % UBUFSIZE))? tmp_len:(UBUFSIZE - (nread % UBUFSIZE));
  memmove(data, &userbuf->data[nread % UBUFSIZE], bytes_to_read);
  nread += bytes_to_read;
  memmove(data + bytes_to_read, &userbuf->data[nread % UBUFSIZE], tmp_len - bytes_to_read);
  nread += tmp_len - bytes_to_read;
  *len = tmp_len;
  
  atomic_store(&userbuf->nread, nread);
  return 0;
}
