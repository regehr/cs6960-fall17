#include "types.h"
#include "stat.h"
#include "user.h"

#define BUFSIZE 1000

int main(){

  int pid = fork();
  if(pid == 0){
    // child
    int ret = buf_setup();
    if(0 != ret){
      printf(1, "error setting up buf in parent\n");
      exit();
    }
    char buf[BUFSIZE];
    int *len = 0;
    ret = buf_get(buf, len, BUFSIZE);
    if(0 == ret){
      printf(1, "got %s\n", buf);
    } else {
      printf(1, "Error receiving!\n");
    }
  } else if(pid >0){
    // parent
    int ret = buf_setup();
    if(0 != ret){
      printf(1, "error setting up buf in parent\n");
      exit();
    }
    ret = buf_put("hello", 6);
    if(0 == ret){
      printf(1, "sent hello\n");
    } else {
      printf(1, "error sending\n");
    }
  } else {
    // error
    printf(1, "Error forking\n");
  }

  exit();
}
