#include "types.h"
#include "stat.h"
#include "user.h"

#define BUF_SIZE 1024
#define BYTES_TO_TEST 1024*1024

void incr(unsigned char *i){
  *i = (*i + 5) % 256;
}

int main(){

  int pid;
  int pipe_fds[2];
  unsigned char compare = 0;
  unsigned char buf[BUF_SIZE];
  int writesizes[3];
  int readsizes[3];
  int sizecounter = 0;

  if(pipe(pipe_fds) != 0){
    printf(1, "pipe syscall failed\n");
    exit();
  }
  int to_pipe = pipe_fds[1];
  int from_pipe = pipe_fds[0];

  pid = fork();

  if (pid == 0){
    // child -- write

    // use various write sizes
    writesizes[0] = (uptime() + 1234) % 256 + 5;
    writesizes[1] = 256;
    writesizes[2] = (uptime() + 4567) % 1000 + 5;
    int total_bytes = 0;
    while(total_bytes < BYTES_TO_TEST){
      int bytes_this_round = 0;
      int ws = writesizes[sizecounter++ % 3];
      //printf(1, "trying to write %d bytes\n", ws);
      for(int i = 0; i<ws; ++i){
        buf[i] = compare;
        incr(&compare);
      }
      while(bytes_this_round < ws && total_bytes < BYTES_TO_TEST){
        int this_write = write(to_pipe, buf+bytes_this_round, ws-bytes_this_round);
        if (this_write < 0){
          printf(1, "error writing\n");
          exit();
        }
        bytes_this_round += this_write;
        total_bytes += this_write;
      }
    }
    close(to_pipe);
    printf(1, "done writing\n");
  } else if (pid > 0){
    // parent -- read

    // use various read sizes
    readsizes[0] = 256;
    readsizes[1] = (uptime() + 654) % 333 + 5;
    readsizes[2] = (uptime() + 7489) % 999 + 5;
    int total_bytes = 0;
    while(total_bytes < BYTES_TO_TEST){
      int rs = readsizes[sizecounter++ % 3];
      //printf(1, "trying to read %d bytes\n", rs);
      int read_this_round = read(from_pipe, buf, rs);
      for(int i = 0; i<read_this_round; ++i){
        if (buf[i] != compare){
          printf(1, "comparison failed when reading\n");
          exit();
        }
        //printf(1, "read byte: %c\n", buf[i]);
        incr(&compare);
      }
      total_bytes += read_this_round;
    }
    close(from_pipe);
    printf(1, "done reading, no errors\n");
    int childpid = wait();
    printf(1, "child with pid %d finished.  Now exiting.\n", childpid);
  } else {
    // fork error
    printf(1, "fork syscall failed\n");
    exit();
  }

  exit();
}
