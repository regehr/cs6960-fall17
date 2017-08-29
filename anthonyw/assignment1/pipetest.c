#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

/*
 * Taken from https://en.wikipedia.org/wiki/Xorshift#Example_implementation 
 */
unsigned xorshift32(unsigned state[static 1])
{
	unsigned x = state[0];
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	state[0] = x;
	return x;
}

//Should only be called from a fork
void write_or_error(int fd, void *buffer, int size)
{
  if(write(fd, buffer, size) != size){
    printf(1, "incomplete write");
    exit();
  }
}


char buf[8192];
void
basictest(void)
{
  int pid, i, n, total;
  int fds[2];
  unsigned rng_seed[1];
  rng_seed[0] = 5;

  if(pipe(fds) != 0){
    printf(1, "pipe() failed\n");
    exit();
  }

  pid = fork();
  if(pid == 0) { //child 
    close(fds[0]);
    for(n = 0; n < 5; ++n){
      for(i = 0; i < 1023; ++i){
        buf[i] = xorshift32(rng_seed) & 0xFF;
      }
      if(write(fds[1], buf, 1023) != 1023){
        printf(1, "pipe child: incomplete write\n");
        exit();
      }
    }
    exit();
  } else if(pid > 0){
    close(fds[1]);
    total = 0;
    //Use a while loop to deal with partial reads
    while((n = read(fds[0], buf, 1023)) > 0){
      for(i = 0; i < n; i++){
        int rng_res = (xorshift32(rng_seed) & 0xff);
        if((buf[i] & 0xff) != rng_res){
          printf(1, "pipe parent: validation error on read\n");
          printf(1, "  got %d, expected %d\n",buf[i] & 0xFF, rng_res);
          exit();
        }
      }
      total += n;
    }
    if(total != 5 * 1023) {
      printf(1, "wrong number of bytes read: %d\n", total);
      exit();
    }
    close(fds[0]);
    wait();
  } else {
    printf(1, "fork() failed\n");
    exit();
  }
  printf(1, "basic pipe test succeeded\n");
}

#define NUM_BYTES 2048
#define NUM_ITERS 1024

/*
 * Measure the speed of writing to a pipe
 */
void
speedtest(void)
{
  int start_ticks, end_ticks, i, n, pid;
  char buffer[NUM_BYTES];
  int fds[2];
  unsigned rng_seed[1];

  if(pipe(fds) != 0){
    printf(1, "speedtest: error during pipe\n");
    exit();
  }

  pid = fork();
  if(pid == 0){ //child
    close(fds[0]);
    rng_seed[0] = 20;
    //Set up array
    for(i = 0; i < NUM_BYTES; i++){
      buffer[i] = xorshift32(rng_seed) & 0xFF;
    }
    //Do the timing
    start_ticks = uptime();
    for(i = 0; i < NUM_ITERS; i++){
      write(fds[1], buffer, NUM_BYTES);
    }
    end_ticks = uptime();
    printf(1, "wrote %d bytes in %d ticks\n", NUM_BYTES * NUM_ITERS, (end_ticks-start_ticks));
    exit();
  } else if (pid > 0){
    close(fds[1]);
    while((n = read(fds[0], buffer, NUM_BYTES)) > 0){
      //Just read all the bytes
    }
    wait();
    exit();
  } else {
    printf(1, "fork() failed\n");
    exit();
  }
}

int main(int argc, char **argv)
{
  basictest();
  speedtest();
  exit();
}

