#include "types.h"
#include "user.h"

//Number of writes
#define N 1<<18

void printf(int fd, char* s, ...) {
    write(fd, s, strlen(s));
}

/**
 * Simple prng
 */
uint random_int32(uint x)
{
	static uint state = 0;
    if(state == 0) state = x;
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

void pipetest() {
  int pipefd[2];

  if( pipe(pipefd) < 0) {
    printf(1,"Could not create pipe");
    exit();
  }
  //seed the prng
  random_int32(N);

  int pid = fork();

  if(pid < 0) {
      printf(1, "Could not fork\n");
      exit();
  } 
  
  else if(pid != 0) { //Parent process
      uint random;
      for(uint i=0;i<N;i++) {
          random = random_int32(1);
          if( write(pipefd[1], &random, sizeof(random)) < 0) {
              printf(1,"Could not write to the pipe");
              exit();
          } 
      }

      wait();
      close(pipefd[1]);
      exit();
  }
  
  else {    //Child process
      uchar buf[4];
      int n, total = 4, nbytes, bad_pipe = 0;
      for(uint i=0;i<N;i++) {
          n = 0;
          nbytes = 0;
          while(nbytes < total) {
              n = read(pipefd[0], buf+nbytes, total-nbytes);
              if(n <= 0) {
                  printf(1,"Error reading from pipe");
                  exit();
              }
              nbytes += n;
          }

          if(*(uint*)&buf != random_int32(1)) {
              bad_pipe = 1;
              break;
          }
      }
      if(bad_pipe) {
          printf(1, "Bad pipe\n");
      }
      else {
          printf(1, "Good pipe\n");
      }

      close(pipefd[0]);
      exit();
  }
}

int main() {
    pipetest();
    exit();
}
