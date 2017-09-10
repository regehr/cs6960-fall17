// pipetest.c by Zhengyang Liu (liuz@cs.utah.edu)
// push one megabyte random data through xv6 pipe

#include "types.h"
#include "stat.h"
#include "user.h"

#define BUF_LEN 1024 * 1024

static unsigned state[1] = {1};

unsigned xorshift32(unsigned state[static 1])
{
  unsigned x = state[0];
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state[0] = x;
  return x;
}

int main()
{
  char *buf_send = malloc(BUF_LEN);
  char* buf_recv = malloc(BUF_LEN);
  int p[2];
  int pid;
  unsigned n;

  // Generate a random buffer for write process.
  for (unsigned i = 0 ; i < BUF_LEN; i ++) {
    buf_send[i] = (char)xorshift32(state);
  }
  
  if(pipe(p) != 0){
    printf(1, "Failed Creating Pipe\n");
    exit();
  }
  
  pid = fork();
  if (pid == 0) { // Write process
    close (p[0]);
    if(write(p[1], buf_send, BUF_LEN) != BUF_LEN){
        printf(1, "Write Error\n");
        exit();
    }
  }
  else if (pid > 0) { // Read process
    close (p[1]);

    
    unsigned buffer_index = 0;
    char buf[1024];
    while((n = read(p[0], buf, 1024)) > 0){
      for (unsigned i = 0; i < n ; i++)
        buf_recv[buffer_index ++] = buf[i];
    }
    for (int i = 0 ; i < BUF_LEN; i ++)
      if(buf_recv[i] != buf_send[i])
      {
        printf(1, "Error Byte Found\n");
        exit();
      }
    close (p[0]);
    printf(1, "Pipe Test Passed!\n");
    wait();
  }
  exit();
}
