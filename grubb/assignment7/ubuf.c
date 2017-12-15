#include "types.h"
#include "user.h"
#include "memlayout.h"

// copied from dongx

#define PGSIZE 4096
#define BUFSIZE ((SHAREDSIZE * PGSIZE) - (2 * sizeof(int)))

static struct {
    int nread, nwrite;
    char data[BUFSIZE];
} *ringbuf;

static inline int min(int a, int b) {
    return a < b ? a : b;
}

static inline int atomic_load(int* x) {
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

static inline void atomic_store(int* x, int val) {
    __atomic_store_n(x, val, __ATOMIC_SEQ_CST);
}

int buf_setup(void) {
    if (ringbuf) return -1;
    ringbuf = setup_shared();
    if (!ringbuf) return -1;
    return 0;
}

// The reason we want to align read write pointer to sizeof(int) is because
// we don't want to wrap the integer for message size or fail to have enough
// space for it as well. This is required for both read and write side since
// either side may leave a unaligned pointer.
int buf_put(char* data, int len) {
    int nread = atomic_load(&(ringbuf->nread));
    int nwrite = (atomic_load(&(ringbuf->nwrite)) + 3) & ~0x03;

    if (BUFSIZE - nwrite + nread < len + sizeof(int)) return -1;

    memmove(&ringbuf->data[nwrite % BUFSIZE], &len, sizeof(int));
    nwrite += sizeof(int);

    int bytes_to_write = min(len, BUFSIZE - (nwrite % BUFSIZE));
    memmove(&ringbuf->data[nwrite % BUFSIZE], data, bytes_to_write);
    nwrite += bytes_to_write;
    memmove(&ringbuf->data[nwrite % BUFSIZE], data + bytes_to_write, len - bytes_to_write);
    nwrite += len - bytes_to_write;

    atomic_store(&ringbuf->nwrite, nwrite);
    return 0;
}

int buf_get(char *data, int *len, int maxlen) {
    int nread = (atomic_load(&(ringbuf->nread)) + 3) & ~0x03;
    int nwrite = atomic_load(&(ringbuf->nwrite));

    if (nwrite == nread) return -1;
    int tmp_len;
    memmove(&tmp_len, &ringbuf->data[nread % BUFSIZE], sizeof(int));
    if (tmp_len > maxlen) return -2;
    nread += sizeof(int);

    int bytes_to_read = min(tmp_len, BUFSIZE - (nread % BUFSIZE));
    memmove(data, &ringbuf->data[nread % BUFSIZE], bytes_to_read);
    nread += bytes_to_read;
    memmove(data + bytes_to_read, &ringbuf->data[nread % BUFSIZE], tmp_len - bytes_to_read);
    nread += tmp_len - bytes_to_read;
    *len = tmp_len;

    atomic_store(&ringbuf->nread, nread);
    return 0;
}
