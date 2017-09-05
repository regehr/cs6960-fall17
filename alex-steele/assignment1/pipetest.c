
// pipetest.c by Alex Steele
//   - one reader and one writer process
//   - (8192 * (8192 + 1) / 2) / 5 = ~6.7 Mb written/read
//   - read and write sizes vary from 1 to 8192 and are decoupled
//   - all bytes read are checked to match those written

#include "types.h"
#include "stat.h"
#include "user.h"

char data[8192];

int total_bytes = 6713344;

void
error(char *msg)
{
  printf(2, msg);
  exit();
}

int
main(void)
{
  int fds[2];
  int pid;
  int i, nw, n;
  int wsize, rsize;
  int next_byte;
  int total_read;

  printf(1, "pipe test\n");
  if (pipe(fds) != 0) {
    error("pipe() failed\n");
  }
  pid = fork();
  if (pid < 0) {
    error("fork() failed\n");
  }
  if (pid == 0) {
    if (close(fds[0])) {
      error("close() failed\n");
    }
    for (i = 0; i < sizeof(data); i++) {
      data[i] = (i % 128);
    }
    for (wsize = 1; wsize <= sizeof(data); wsize += 5) {
      nw = 0;
      while (nw < wsize) {
        n = write(fds[1], data + nw, wsize - nw);
        if (n < 0) {
          error("write() failed\n");
        }
        nw += n;
      }
    }
    if (close(fds[1])) {
      error("close() failed\n");
    }
  } else {
    if (close(fds[1])) {
      error("close() failed\n");
    }
    total_read = 0;
    rsize = 1;
    wsize = 1;
    next_byte = 0;
    while ((n = read(fds[0], data, rsize)) > 0) {
      total_read += n;
      for (i = 0; i < n; i++) {
        if (data[i] != (next_byte % 128)) {
          error("FAIL - read incorrect byte\n");
        }
        next_byte++;
        if (next_byte == wsize) {
          wsize += 5;
          next_byte = 0;
        }
      }
      rsize += 31;
      if (rsize > sizeof(data)) {
        rsize = 1;
      }
    }
    if (n < 0) {
      error("read() failed\n");
    }
    if (total_read != total_bytes) {
      error("FAIL - read too few bytes\n");
    }
    if (close(fds[0])) {
      error("close() failed\n");
    }
    wait();
    printf(1, "PASS\n");
  }

  exit();
}
