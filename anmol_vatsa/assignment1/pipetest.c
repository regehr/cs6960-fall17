#include "types.h"
#include "user.h"

//Number of writes
#define N 1<<12

//#define MAX_WR_UNITS 0x00100000

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

void pipetest_multiple() {
  printf(1, "Test Multiple Read/Writes...\n");
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
          unsigned int length = (i +1)*size;
          //printf(1, "Writer: writing %d bytes\n", length+1);
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
      return;
  }

  else {    //Child process
      char *buf;
      unsigned int n, nbytes;
      unsigned int size = sizeof(unsigned int);

      //DO N iterations of read
      for(unsigned int i=0;i<N;i++) {
          //read length+1 bytes
          unsigned int length = (i+1)*size;
          //printf(1,"Reader: reading %d bytes\n", length+1);
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
              printf(1, "...Test Multiple Read/Writes Failure\n");
              free(inbuf);
              free(buf);
              close(pipefd[0]);
              exit();
          }

          //free memory
          free(inbuf);
          free(buf);
      }

      printf(1, "...Test Multiple Read/Writes Success\n");

      close(pipefd[0]);
      exit();
  }
}
void pipetestsingle() {
  printf(1, "Testing Single read...\n");
  int pipefd[2];

  if( pipe(pipefd) < 0) {
    printf(1,"Could not create pipe");
    exit();
  }
  //seed the prng
  random_int32(N);

  unsigned int length = 513;
  char obuf[length];

  for(unsigned int j=0;j<length; j++) {
    obuf[j] = (char)random_int32(1);
  }

  int pid = fork();

  if(pid < 0) {
      printf(1, "Could not fork\n");
      exit();
  }

  else if(pid != 0) { //Parent process
    close(pipefd[0]);
    int nbytes = 0, n = 0;
    //take care of short writes
    while(nbytes < length) {
      n=write(pipefd[1], obuf+nbytes, length-nbytes);
      if(n <= 0) {
        printf(1,"Could not write to the pipe");
        exit();
      }
      nbytes += n;
    }
    printf(1, "Writer: Wrote %d bytes\n", nbytes);
    wait();
    close(pipefd[1]);
    return;
  }
  else {    //Child process
    close(pipefd[1]);
    char inbuf[length];
    unsigned int n, nbytes;

    n = 0;
    nbytes = 0;
    //take care of short reads
    while(nbytes < length) {
      n = read(pipefd[0], inbuf+nbytes, length-nbytes);
      if(n <= 0) {
          printf(1,"Error reading from pipe");
          exit();
      }
      nbytes += n;
    }
    printf(1, "Reader: Read %d bytes\n", nbytes);
    //compare what you read with what you should have read
    for(int i=0;i<length;i++) {
      if(obuf[i]!=inbuf[i]) {
        printf(1, "Bad pipe. %d byte does not match\n", i);
        close(pipefd[0]);
        exit();
      }
    }

    printf(1, "...Test Single read Success\n");

    close(pipefd[0]);
    exit();
  }
}

void pipetest_short_read() {
  printf(1, "Testing short read...\n");
  int pipefd[2];

  if( pipe(pipefd) < 0) {
    printf(1,"Could not create pipe");
    exit();
  }
  //seed the prng
  random_int32(N);

  unsigned int length = 128;
  char obuf[length];

  for(unsigned int j=0;j<length; j++) {
    obuf[j] = (char)random_int32(1);
  }

  int pid = fork();

  if(pid < 0) {
      printf(1, "Could not fork\n");
      exit();
  }

  else if(pid != 0) { //Parent process
    close(pipefd[0]);
    int nbytes = 0, n = 0;
    //take care of short writes
    while(nbytes < length) {
      n=write(pipefd[1], obuf+nbytes, length-nbytes);
      if(n <= 0) {
        printf(1,"Could not write to the pipe");
        exit();
      }
      nbytes += n;
    }

    wait();
    close(pipefd[1]);
    return;
  }
  else {    //Child process
    close(pipefd[1]);
    char inbuf[length];
    int n = read(pipefd[0], inbuf, length+length);
    if(n <= 0) {
      printf(1,"Error reading from pipe n=%d\n", n);
      close(pipefd[0]);
      exit();
    } else if(n != length) {
      printf(1, "Read wrong number of bytes n=%d\n", n);
      close(pipefd[0]);
      exit();
    }

    //compare what you read with what you should have read
    for(int i=0;i<length;i++) {
      if(obuf[i]!=inbuf[i]) {
        printf(1, "Bad pipe. %d byte does not match\n", i);
        close(pipefd[0]);
        exit();
      }
    }

    printf(1, "...Test Short read Success\n");

    close(pipefd[0]);
    exit();
  }
}

int main() {
  pipetest_multiple();
  pipetestsingle();
  pipetest_short_read();
  exit();
}
