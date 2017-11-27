#include "types.h"
#include "user.h"

#define PAGE_SIZE 4096
#define BUF_HEADER_SIZE (2 * sizeof(int))
#define BUF_SIZE (PAGE_SIZE * 8 - BUF_HEADER_SIZE)

struct {
  uint num_read;
  uint num_write;
  char data[BUF_SIZE];
} *buffer;

uint 
atomic_load(uint *ptr)
{
  return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

int
atomic_store(uint *ptr, uint value)
{
  __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
  return 0;
}

int
buf_setup(void)
{
  if (buffer) {
    printf(2, "Buffer already setup!\n");
    return -1;
  }

  buffer = mapshared();

  if (buffer) {
    return 0;
  } else {
    printf(2, "Unable to allocate memory for buffer!\n");
    return -1;
  }
}

int
min(int x, int y) {
  return x < y ? x : y;
}

int 
buf_put(char *data, uint len)
{
  // Retrieve the current read and write indices.
  uint current_write = atomic_load(&buffer->num_write);
  uint current_read = atomic_load(&buffer->num_read);
  
  // Make sure the buffer has enough space
  if (current_write - current_read + len > BUF_SIZE) {
    printf(2, "Not enough space in buffer!\n");
    return -1;
  }

  // Write the message length
  buffer->data[current_write % BUF_SIZE] = len;
  current_write += sizeof(int);

  // Write out the message
  uint write_size = min(len, BUF_SIZE - (current_write % BUF_SIZE));
  memmove(&buffer->data[current_write % BUF_SIZE], data, write_size);
  current_write += write_size;
  memmove(&buffer->data[current_write % BUF_SIZE], data + write_size, len - write_size);
  current_write += len - write_size;

  // Update the write index
  atomic_store(&buffer->num_write, current_write);

  return 0;
}

int
buf_get(char *data, uint *len, uint maxlen)
{
  // Retrieve the current read and write indices.
  uint current_write = atomic_load(&buffer->num_write);
  uint current_read = atomic_load(&buffer->num_read);
  
  // Make sure there is something to read
  if (current_read >= current_write) {
    printf(2, "Nothing to read!\n");
    return -1;
  }

  // Read the message length
  uint message_length = buffer->data[current_read % BUF_SIZE];

  // If the message is larger than what we're willing to read, return a specific error code.
  if (message_length > maxlen) {
    printf(2, "Message too large to read!\n");
    return -2;
  }
  current_read += 4;

  // Copy the message out.
  uint read_size = min(message_length, BUF_SIZE - (current_read % BUF_SIZE));
  memmove(&buffer->data[current_read % BUF_SIZE], data, read_size);
  current_read += read_size;
  memmove(&buffer->data[current_read % BUF_SIZE], data + read_size, message_length - read_size);
  current_read += message_length - read_size;

  // Update the write index
  atomic_store(&buffer->num_read, current_read);

  // Update the output pointer
  *len = message_length;

  return 0;
}

