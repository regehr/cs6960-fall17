
// Basic test intended to check process state handling
// and process scheduling in proc.c.

#include "types.h"
#include "user.h"
#include "fcntl.h"

#define NPROCS 32

int main(void)
{
  int pid;
  int i;

  printf(1, "schedtest: %d child procs\n", NPROCS);
  for (i = 0; i < NPROCS; i++) {
    pid = fork();
    if (pid == 0) {
      char fname[7];
      char buf[256];
      int fd;

      // Make system calls and do work to change
      // proc->state with hopes of running the scheduler
      // code with a variety of system states.
      pid = getpid();
      sleep(pid % 20);

      strcpy(fname, "test_");
      fname[5] = (char) (pid % 128);
      fname[6] = 0;

      fd = open(fname, O_CREATE|O_RDWR);
      if (fd < 0) {
        printf(2, "could not open %s\n", fname);
        exit();
      }
      memset(buf, 1, sizeof(buf));
      write(fd, buf, sizeof(buf));
      write(fd, buf, sizeof(buf));

      sleep(pid % 20);
      
      write(fd, buf, sizeof(buf));
      close(fd);
      unlink(fname);

      sleep(pid % 10);
      
      exit();
    }
  }
  while ((pid = wait()) != -1)
    printf(1, "proc %d finished\n", pid);
  printf(1, "success\n");
  exit();
}
