#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

uint32_t xorshift32(uint32_t state) {
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  return x;
}

void testBasicPipeThroughPut() {
  int fd[2];
  uint32_t seed = 313;
  int i, j;
  pipe(fd);
  int BUFFER_LEN = 4;
  char buffer[BUFFER_LEN];
  
  if (fork() == 0) {
    close(fd[1]);
    for (i = 0; i < 1000; i++) {
      for (j = 0; j < BUFFER_LEN; j++) {
        seed = xorshift32(seed);
        buffer[j] = (unsigned char) seed & 0xfff;
      }
      char temp[BUFFER_LEN];
      int nbytes = read(fd[0], temp, sizeof(temp));
      for (j = 0; j < BUFFER_LEN; j++) {
        if ((unsigned char) temp[j] != (unsigned char) buffer[j]) {
          printf("Correct: %x, Actual: %x\n", buffer[j], temp[j]);
        }
      } 
    }
  } else {
    close(fd[0]);
    for (i = 0; i < 1000; i++) {
      for (j = 0; j < BUFFER_LEN; j++) {
        seed = xorshift32(seed);
        buffer[j] = (unsigned char) seed & 0xfff;
      }
      write(fd[1], buffer, sizeof(buffer));
    }
  }
}

int main() {
  testBasicPipeThroughPut();
}
