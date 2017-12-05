#include "types.h"
#include "user.h"
#include "memlayout.h"

#define PAGE_SIZE 4096
#define BUFFER_SIZE ((SHAREDPAGES * PAGE_SIZE) - (2 * sizeof(int)))    //
#define MINM(a, b) ((a) < (b) ? (a) : (b))

static struct {
    int nread_bytes;
    int nwrite_bytes;
    char data[BUFFER_SIZE];
} *ring_buffer;

static int atomic_load(int* x) {
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

static void atomic_store(int* x, int val) {
    return __atomic_store_n(x, val, __ATOMIC_SEQ_CST);
}

int buf_setup(void) {
    if (ring_buffer)
        return -1;
    ring_buffer = map_shared();
    if (!ring_buffer) 
        return -1;
    return 0;
}

int buf_read(char *data, int *len, int maxlen) {
    
    int nr = (atomic_load(&(ring_buffer->nread_bytes)) + 3) & ~0x03;
    int nw = atomic_load(&(ring_buffer->nwrite_bytes));

    //empty
    if (nw == nr) 
        return -1;
    int nr1 = *((int*)(&ring_buffer->data[nread_bytes % BUFFER_SIZE]));
    if (nr1 > maxlen)
        return -2;
    nr += 4;

    int nr2 = MINM(nr1, BUFFER_SIZE - (nr % BUFFER_SIZE));
    memmove(data, &ring_buffer->data[nr % BUFFER_SIZE], nr2);
    nr += nr2;
    memmove(data + nr2, &ring_buffer->data[nr % BUFFER_SIZE], nr1 - nr2);
    nr += nr1 - nr2;
    *len = nr1;

    atomic_store(&ring_buffer->nread_bytes, nr);
    return 0;
}

int buf_write(char* data, int len) {
    int nr = atomic_load(&(ring_buffer->nread_bytes));
    int nw = (atomic_load(&(ring_buffer->nwrite_bytes)) + 3) & ~0x03;

    if (BUFFER_SIZE - nw + nr < len + sizeof(int))
     return -1;

    *((int*)(&ring_buffer->data[nw % BUFFER_SIZE])) = len;
    nw += 4;

    int nw1 = MINM(len, BUFFER_SIZE - (nw % BUFFER_SIZE));
    memmove(&ring_buffer->data[nw% BUFFER_SIZE], data, nw1);
    nw += nw1;
    memmove(&ring_buffer->data[nw % BUFFER_SIZE], data + nw1, len - nw1);
    nw += len - nw1;

    atomic_store(&ring_buffer->nwrite_bytes, nw);
    return 0;
}


