#include "types.h"
#include "user.h"

//Number of writes
#define N 1<<18

#define MAX_WR_UNITS 0x00100000

void printf(int fd, char* s, ...) {
    write(fd, s, strlen(s));
}

/**
 * Simple prng
 */
unsigned int random_int32(unsigned int x)
{
	static unsigned int state = 0;
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
      unsigned int random;
      unsigned int size = sizeof(unsigned int);
      char* obuf;
      //Do N iterations of write
      for(unsigned int i=0;i<N;i++) {
          //write length+1 bytes
          unsigned int length = ((i & MAX_WR_UNITS)+1)*size;
          obuf = malloc(length+1);
          //fill up the write buffer with length random bytes
          for(unsigned int j=0;j<length; j+=size) {
            random = random_int32(1);
            for(int k=0;k<size;k++) {
                obuf[j+k] = random >> (8*k);
            }
          }
          obuf[length] = 0;

          int nbytes = 0, n = 0;
          //take care of short writes
          while(nbytes < length+1) {
              n=write(pipefd[1], obuf+nbytes, length+1-nbytes);
              if(n <= 0) {
                printf(1,"Could not write to the pipe");
                exit();
              }
              nbytes += n;
          }

          //free memory
          free(obuf);  
      }

      wait();
      close(pipefd[1]);
      exit();
  }
  
  else {    //Child process
      char *buf;
      unsigned int n, nbytes, bad_pipe = 0;
      unsigned int size = sizeof(unsigned int);

      //DO N iterations of read
      for(unsigned int i=0;i<N;i++) {
          //read length+1 bytes
          unsigned int length = ((i & MAX_WR_UNITS)+1)*size;  
          n = 0;
          nbytes = 0;
          buf = malloc(length+1);

          //take care of short reads
          while(nbytes < length+1) {
              n = read(pipefd[0], buf+nbytes, length+1-nbytes);
              if(n <= 0) {
                  printf(1,"Error reading from pipe");
                  exit();
              }
              nbytes += n;
          }

          //build what you read yourself from the same prng to match
          char *inbuf = malloc(length+1);
          int random=0;
          for(unsigned int j=0;j<length; j+=size) {
            random = random_int32(1);
            for(int k=0;k<size;k++) {
                inbuf[j+k] = random >> (8*k);
            }
          }
          inbuf[length] = 0;

          //compare what you read with what you should have read
          if(strcmp(inbuf, buf)!= 0) {
              bad_pipe = 1;
              free(inbuf);
              free(buf);
              break;
          }

          //free memory
          free(inbuf);
          free(buf);
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
    return 0;
}
