#include "types.h"
#include "stat.h"
#include "user.h"

// borrowed and modified from usertests.c
unsigned long
rand(unsigned long randstate)
{
  randstate = randstate * 1664525 + 1013904223;
  return randstate;
}

int main() {
  int fd[2];
  pipe(fd); 

  unsigned long streamstate = 1;
 
  if (fork() == 0) {
    //child
    close(fd[1]);
   
    unsigned long procstate = 2;
 
    int i,j;
    for (i = 0; i < 1000000; ) {
      procstate = rand(procstate);
      int BUF_SIZE = (int) procstate % 1000;
      if (BUF_SIZE < 0) {
        BUF_SIZE = BUF_SIZE * -1;
      }
      char buf[BUF_SIZE];
      int nbytes = read(fd[0], buf, sizeof(buf));
      for (j = 0; j < nbytes; j++) {
        streamstate = rand(streamstate);
        char correctChar = (unsigned char) streamstate & 0xfff;
        if (buf[j] != correctChar) {
          printf(1, "Correct: %x, Actual: %x\n", correctChar, buf[j]);
        }
      }
      i = i + nbytes;
    }
  
  } else {
    //parent
    close(fd[0]);

    unsigned long procstate = 3;

    int i,j;
    for (i = 0; i < 1000000; ) { 
      procstate = rand(procstate);
      int BUF_SIZE = (int) procstate % 1000;
      if (BUF_SIZE < 0) {
        BUF_SIZE = BUF_SIZE * -1;
      }
      char buf[BUF_SIZE];
      for (j = 0; j < BUF_SIZE; j++) {
        streamstate = rand(streamstate);
        char correctChar = (unsigned char) streamstate & 0xfff;
        buf[j] = correctChar;
      }
      int nbytes = write(fd[1], buf, sizeof(buf));
      i = i + nbytes;
      // handle short writes
      while (nbytes < BUF_SIZE) {
        BUF_SIZE = BUF_SIZE - nbytes;
        char temp[BUF_SIZE];
        for (j = 0; j < BUF_SIZE; j++) {
          temp[j] = buf[j + nbytes];
        }
        nbytes = write(fd[1], temp, sizeof(temp));
        i = i + nbytes;
      }
    }
    wait();
  }

  exit();
}
