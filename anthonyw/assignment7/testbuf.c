#include "types.h"
#include "user.h"

#define MY_BUF_SIZE 128
#define TRIALS 20

int main()
{
  int pid;
  int res;
  int trials;
  char buf[MY_BUF_SIZE];

  res = buf_setup();
  if(res < 0){
    printf(1, "buffer setup failed\n");
    exit();
  }

  pid = fork();
  if(pid == 0){
    int i;
    for(i = 0; i < MY_BUF_SIZE; ++i){
      buf[i] = (char) (i & 0xFF);
    }
    for(trials = 0; trials < TRIALS; trials++){
      buf_put(buf, 64);
    }
    exit();

  } else if(pid > 0){
    int i;
    uint len;
    wait();
    
    for(trials = 0; trials < TRIALS; trials++){
      res = buf_get(buf, &len, MY_BUF_SIZE);
      if(res < 0){
        printf(1, "buf_get failed: error %d\n", res);
        exit();
      }
      for(i = 0; i < len; ++i){
        if(buf[i] != (char) (i & 0xFF)){
          printf(1, "error: expected %d, got %d\n", (i  & 0xFF), buf[i]);
        }
      }
    }

  } else {
    printf(1, "fork() failed:\n");
    exit();
  }

  printf(1, "successfully finished!\n");

  exit();
  return 0;
}
