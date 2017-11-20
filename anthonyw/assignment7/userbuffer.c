// Assignment 7 ring buffer code.
#include "types.h"
#include "user.h"

#define NUM_PAGES 8
#define PAGESIZE 4096
#define SIZE_OVERHEAD (sizeof(uint))
#define SHAREDMEM_SIZE (NUM_PAGES * PAGESIZE)
//This is kind of ugly, but oh well.
#define RINGBUF_SIZE (SHAREDMEM_SIZE - 2 * sizeof(uint))

/*memcpy())*/

void *memcpy(void *restrict dest, const void *restrict src, uint n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	for (; n; n--) *d++ = *s++;
	return dest;
}


struct ring_buffer {
  uint nread;
  uint nwrite;
  char buf[RINGBUF_SIZE];
};

static struct ring_buffer *ringbuf;

/*
 * Set up the ring buffer.
 */
int buf_setup(void)
{
  if(ringbuf){
    return -1;
  }
  ringbuf = (struct ring_buffer *) memshared();
  if(!ringbuf){
    return -1;
  }

  /*Should be initialized to zero when it's given to us, but just in case...*/
  ringbuf->nread = 0;
  ringbuf->nwrite = 0;
  *(uint *)(ringbuf->buf + 0) = 0;

  return 0;
}

/*TODO: make loads/stores to nwrite/nread atomic using GCC primitives*/
int buf_put(char *data, uint len)
{
  uint space_needed = len + SIZE_OVERHEAD;
  uint nread;


  if(!ringbuf){
    return -3;
  }

  /*Atomically retrieve our value for nread*/
  __atomic_load(&(ringbuf->nread), &nread, __ATOMIC_SEQ_CST);
  if(ringbuf->nwrite + space_needed <= nread + RINGBUF_SIZE){
    if(ringbuf->nwrite % RINGBUF_SIZE + space_needed <= RINGBUF_SIZE){
      /*Write size*/
      *(uint *)(&(ringbuf->buf[ringbuf->nwrite % RINGBUF_SIZE])) = len;
      /*Do just one copy*/
      memcpy(&(ringbuf->buf[(ringbuf->nwrite + SIZE_OVERHEAD) % RINGBUF_SIZE]), data, len);
    } else {
      uint bytes_til_end = RINGBUF_SIZE - ringbuf->nwrite % RINGBUF_SIZE;
      /*Write size*/
      *(uint *)(&(ringbuf->buf[ringbuf->nwrite % RINGBUF_SIZE])) = len;
      /*Do two copies*/
      memcpy(&(ringbuf->buf[(ringbuf->nwrite + SIZE_OVERHEAD) % RINGBUF_SIZE]), data, bytes_til_end - SIZE_OVERHEAD);
      memcpy(&(ringbuf->buf[(ringbuf->nwrite + bytes_til_end) % RINGBUF_SIZE]), data + (bytes_til_end - SIZE_OVERHEAD), len - (bytes_til_end - SIZE_OVERHEAD));
    }
  } else {
    /*Not enough space*/
    return -1;
  }
  /*
  ringbuf->nwrite += space_needed;
  */
  __atomic_store_n(&(ringbuf->nwrite), ringbuf->nwrite + space_needed, __ATOMIC_SEQ_CST);
  return 0;
}

int buf_get(char *data, uint *len, uint maxlen)
{
  uint msg_len;
  uint nwrite;
  if(!ringbuf){
    return -3;
  }

  __atomic_load(&(ringbuf->nwrite), &nwrite, __ATOMIC_SEQ_CST);
  if(ringbuf->nread == nwrite){
    return -1;
  }

  msg_len = *(uint *)(&(ringbuf->buf[ringbuf->nread % RINGBUF_SIZE]));
  if(msg_len > maxlen){
    return -2;
  }

  if(ringbuf->nread % RINGBUF_SIZE + msg_len + SIZE_OVERHEAD <= RINGBUF_SIZE){
    /*Overwrite the length to make sure we don't accidentally use it later*/
    *(uint *)(&(ringbuf->buf[ringbuf->nread % RINGBUF_SIZE])) = 0;
    /*One copy*/
    memcpy(data, &(ringbuf->buf[(ringbuf->nread + SIZE_OVERHEAD) % RINGBUF_SIZE]), msg_len);
  } else {
    uint bytes_til_end = RINGBUF_SIZE - ringbuf->nread % RINGBUF_SIZE;
    /*Overwrite the length to make sure we don't accidentally use it later*/
    *(uint *)(&(ringbuf->buf[ringbuf->nread % RINGBUF_SIZE])) = 0;
    /*Two copies*/
    memcpy(data, &(ringbuf->buf[(ringbuf->nread + SIZE_OVERHEAD) % RINGBUF_SIZE]), bytes_til_end - SIZE_OVERHEAD);
    memcpy(data + (bytes_til_end - SIZE_OVERHEAD), &(ringbuf->buf[(ringbuf->nread + bytes_til_end) % RINGBUF_SIZE]), msg_len - (bytes_til_end - SIZE_OVERHEAD));
  }
  __atomic_store_n(&(ringbuf->nread), ringbuf->nread + msg_len + SIZE_OVERHEAD, __ATOMIC_SEQ_CST);

  *len = msg_len;
  return 0;
}
