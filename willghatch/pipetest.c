#include "types.h"
#include "stat.h"
#include "user.h"

#define BUF_SIZE 1024
#define BYTES_TO_TEST 1024*1024
#define BYTES_PER_WRITER BYTES_TO_TEST/2
#define COMPARE_START 37

void incr(unsigned char *i){
  *i = (*i + 5) % 256;
  // use 0 and 1 as signal values
  if (*i < 2){
    *i += 2;
  }
}

void do_write(unsigned char flag, int to_pipe, int signal_read, int signal_write){
  unsigned char compare = COMPARE_START;
  unsigned char buf[BUF_SIZE];
  int sizecounter = 0;

  // use various write sizes
  int writesizes[3];
  writesizes[0] = (uptime() + 1234) % 256 + 5;
  writesizes[1] = 256;
  writesizes[2] = (uptime() + 4567) % 1000 + 5;
  int total_bytes = 0;
  while(total_bytes < BYTES_PER_WRITER){
    // wait for the other process to finish a pass
    while(!read(signal_read, buf, 1));

    // signal the reader that we're switching writers
    buf[0] = flag;
    while(!write(to_pipe, buf, 1));
    int bytes_this_round = 0;
    int ws = writesizes[sizecounter++ % 3];
    if (ws + total_bytes > BYTES_PER_WRITER){
      ws = BYTES_PER_WRITER - total_bytes;
    }
    //printf(1, "trying to write %d bytes\n", ws);
    for(int i = 0; i<ws; ++i){
      buf[i] = compare;
      incr(&compare);
    }
    while(bytes_this_round < ws && total_bytes < BYTES_PER_WRITER){
      int this_write = write(to_pipe, buf+bytes_this_round, ws-bytes_this_round);
      if (this_write < 0){
        printf(1, "error writing\n");
        exit();
      }
      bytes_this_round += this_write;
      total_bytes += this_write;

    }
    // signal that this process is done with a pass
    while(!write(signal_write, buf, 1));
  }
  close(to_pipe);
}

int main(){

  int pid;
  int pipe_fds[2];
  unsigned char compare[2] = {COMPARE_START,COMPARE_START};
  unsigned char buf[BUF_SIZE];
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

    int signal_fds[2];
    if(pipe(signal_fds) != 0){
      printf(1, "pipe syscall failed\n");
      exit();
    }
    int signal_write = signal_fds[1];
    int signal_read = signal_fds[0];

    // we need to have >1 writer or reader, so let's go ahead and do 2 writers.
    pid = fork();
    if (pid == 0){
      // grandchild
      do_write(0, to_pipe, signal_read, signal_write);
      printf(1, "done writing in grandchild\n");
    } else if (pid > 0){
      // original child, parent of new child
      // signal the child to go first
      while(!write(signal_write, buf, 1));

      do_write(1, to_pipe, signal_read, signal_write);
      printf(1, "done writing in child\n");
      
      int childpid = wait();
      printf(1, "child with pid %d finished (grandchild process).\n", childpid);
    } else {
      // fork error
      printf(1, "fork syscall failed in creating grandchild\n");
      exit();
    }

  } else if (pid > 0){
    // parent -- read

    // use various read sizes
    readsizes[0] = 256;
    readsizes[1] = (uptime() + 654) % 333 + 5;
    readsizes[2] = (uptime() + 7489) % 999 + 5;
    int total_bytes = 0;
    // flag to know which writer we're reading from
    int compareflag = 0;
    while(total_bytes < BYTES_TO_TEST){
      int rs = readsizes[sizecounter++ % 3];
      //printf(1, "trying to read %d bytes\n", rs);
      int read_this_round = read(from_pipe, buf, rs);
      for(int i = 0; i<read_this_round; ++i){
        if (buf[i] < 2){
          // we're switching to read from the other child
          compareflag = buf[i];
          // don't count flag signal bytes against total
          total_bytes--;
        }
        else{
          if (buf[i] != compare[compareflag]){
            printf(1, "comparison failed when reading!\n");
            printf(1, "expected %d, got %d\n", compare[compareflag], buf[i]);
            exit();
          }
          incr(compare+compareflag);
        }
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
