#include "types.h"
#include "memlayout.h"
#include "mmu.h"
#include "user.h"
#include "spinlock.h"
#include "memlayout.h"


#define SHARED_PAGES (8)
#define BUF_SIZE     (SHARED_PAGES*PGSIZE)


#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


static struct {

    int    read_ptr;
    int    write_ptr;
    char * base_page;
    char   data[BUF_SIZE/PGSIZE];

} * ring_buf;


// =================================================================================================
// buf_put() copies len bytes into the ring buffer.
//   Returns 0 on success, else returns -1 if there isn't room.
// =================================================================================================
int
buf_put( char * data
       , uint   len
       )
{
  int reads  =  __atomic_load_n(&(ring_buf->read_ptr),  __ATOMIC_SEQ_CST);
  int writes = (__atomic_load_n(&(ring_buf->write_ptr), __ATOMIC_SEQ_CST) +3) & ~0x03;

  // returns -1 if there isn't room
  if (BUF_SIZE - writes + reads < len + sizeof(int)) {
    return -1;
  }

  memmove(&ring_buf->data[writes % BUF_SIZE], &len, sizeof(int));
  writes += sizeof(int);
  int num_bytes = min(len, BUF_SIZE - (writes % BUF_SIZE));
  memmove(&ring_buf->data[writes % BUF_SIZE], data, num_bytes);
  writes += num_bytes;
  memmove(&ring_buf->data[writes % BUF_SIZE], data + num_bytes, len - num_bytes);
  writes += len - num_bytes;

  __atomic_store_n(&ring_buf->write_ptr, writes, __ATOMIC_SEQ_CST);

  return 0;
}


// =================================================================================================
// buf_get() copies a message out of the ring buffer if one is available.
//   Returns 0 on success, else -1 if no message is available.
//   If there is an available message but its size is larger than maxlen,
//   the message is left in the buffer and this function returns -2.
// =================================================================================================
int
buf_get( char * data
       , uint * len
       , uint   maxlen
       )
{
  int reads  = (__atomic_load_n(&(ring_buf->read_ptr),   __ATOMIC_SEQ_CST) +3) & ~0x03;
  int writes =  __atomic_load_n(&(ring_buf->write_ptr),  __ATOMIC_SEQ_CST);


  // -1 if no message is available
  if (writes == reads) {
    return -1;
  }

  int tmp_len;
  memmove(&tmp_len, &ring_buf->data[reads % BUF_SIZE], sizeof(int));

  // available message but its size is larger than maxlen
  if (tmp_len > maxlen) {
    return -2;
  }

  reads += sizeof(int);
  int bytes_to_read = min(tmp_len, BUF_SIZE - (reads % BUF_SIZE));
  memmove(data, &ring_buf->data[reads % BUF_SIZE], bytes_to_read);
  reads += bytes_to_read;
  memmove(data + bytes_to_read, &ring_buf->data[reads % BUF_SIZE], tmp_len - bytes_to_read);
  reads += tmp_len - bytes_to_read;
  *len = tmp_len;

  __atomic_store_n(&ring_buf->read_ptr, reads, __ATOMIC_SEQ_CST);

  return 0;
}


// =================================================================================================
// buf_setup() allocates the ring buffer (if necessary) and maps it into the calling process's
// memory. The ring buffers should be something like 8 pages of RAM (#define SHAREDPAGES 8).
//   Returns 0 on success, returns -1 if any given process calls this function twice (or if a
//   descendant of a process that already called this function calls it) or if a memory allocation fails.
// =================================================================================================
int
buf_setup(void)
{
  if (ring_buf) {
    return -1;
  }

  ring_buf = map_shared_mem();

  if (!ring_buf) {
    return -1;
  }

  return 0;
}

