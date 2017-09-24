
#include "types.h"
#include "user.h"
#include "fcntl.h"

char *test_file = "testfile.txt";

void
check(int cond, char *s)
{
  if (!cond) {
    printf(2, "error: %s\n", s);
    printf(2, "FAIL\n");
    exit();    
  }
}

void
test_syscalls(void)
{
  int m;
  
  printf(1, "testing basic syscalls\n");
  
  m = mutex_create();
  
  check(m >= 0, "mutex_create() failed");
  check(mutex_lock(m) == 0, "mutex_lock() failed");
  check(mutex_unlock(m) == 0, "mutex_unlock() failed");
  check(mutex_destroy(m) == 0, "mutex_destroy() failed");
}

void
check_write_read(int procnum)
{
  char wbuf[1024]; // bytes written to file
  char rbuf[1024]; // bytes read from file
  int fd;
  int n;

  // xv6's slimmmed down poxix-IO API makes this a little trickier than it
  // otherwise might be. This just checks that each process is able to write and
  // read N bytes at the beginning of the test file without getting stepped on.

  fd = open(test_file, O_WRONLY);
  check(fd > 0, "open() failed\n");
  
  memset(wbuf, (char) procnum, sizeof(wbuf));
  for (n = 0; n < sizeof(wbuf); n += 64) {
    check(write(fd, wbuf + n, 64) == 64, "short write");
  }
  close(fd);

  fd = open(test_file, O_RDONLY);
  check(fd > 0, "open() failed");
  for (n = 0; n < sizeof(rbuf);) {
    int nr = read(fd, rbuf + n, sizeof(rbuf) - n);
    check(nr >= 0, "read failed");
    if (nr == 0) {
      break;
    }
    n += nr;
  }
  close(fd);
  check(n == sizeof(rbuf), "read too few bytes");

  wbuf[sizeof(wbuf)-1] = 0;
  rbuf[sizeof(rbuf)-1] = 0;

  check(strcmp(wbuf, rbuf) == 0, "bytes read != bytes written");
}

void
test_mutual_exclusion(void)
{
  int nprocs = 8;
  int procnum;
  int fd;
  int m; // mutex descriptor

  printf(1, "testing exclusion with file writes\n");

  // Create test file.
  fd = open(test_file, O_CREATE);
  check(fd > 0, "open() failed");
  close(fd);

  m = mutex_create();
  check(m >= 0, "mutex_create() failed");
  for (procnum = 0; procnum < nprocs; procnum++) {
    if (fork() == 0) {
      check(!mutex_lock(m), "mutex_lock() failed");
      check_write_read(procnum);
      check(!mutex_unlock(m), "mutex_unlock() failed");
      exit();
    }
  }
  while (wait() > 0)
    ;
  unlink(test_file);
  check(!mutex_destroy(m), "mutex_destroy() failed");
}

void
test_mutual_exclusion2(void)
{
  int nprocs = 16;
  int procnum;
  int m;
  int i;

  printf(1, "testing exclusion with printf's\n");

  m = mutex_create();
  check(m >= 0, "mutex_create() failed");
  for (procnum = 0; procnum < nprocs; procnum++) {
    if (fork() == 0) {
      check(!mutex_lock(m), "mutex_lock() failed");
      printf(1, "%d inside critical section\n", procnum);
      for (i = 0; i < 5; i++) {
        printf(1, "%d ", i);
      }
      printf(1, "\n");
      check(!mutex_unlock(m), "mutex_unlock() failed");
      exit();
    }
  }
  while (wait() > 0)
    ;
  check(!mutex_destroy(m), "mutex_destroy() failed");
}

void
test_create_multiple(void)
{
  int nmutex = 16;
  int mids[nmutex];
  int i;

  printf(1, "testing creation of multiple mutexes\n");

  for (i = 0; i < nmutex; i++) {
    mids[i] = mutex_create();
  }

  for (i = 0; i < nmutex; i++) {
    check(mids[i] >= 0, "mutex_create() failed");    
  }

  for (i = 0; i < nmutex; i++) {
    check(!mutex_lock(mids[i]), "mutex_lock() failed");
  }

  for (i = 0; i < nmutex; i++) {
    check(!mutex_unlock(mids[i]), "mutex_unlock() failed");
  }

  for (i = 0; i < nmutex; i++) {
    check(!mutex_destroy(mids[i]), "mutex_destroy() failed");
  }
}

void
test_bad_args(void)
{
  printf(1, "testing lock/unlock with bad args\n");

  check(mutex_lock(-1) == -1, "bad lock succeeded\n");
  check(mutex_lock(5) == -1, "bad lock succeeded\n");
  check(mutex_lock(384) == -1, "bad lock succeeded\n");

  check(mutex_unlock(-1) == -1, "bad unlock succeeded\n");
  check(mutex_unlock(5) == -1, "bad unlock succeeded\n");
  check(mutex_unlock(384) == -1, "bad unlock succeeded\n");
}

void
test_destroy(void)
{
  int mid;

  printf(1, "testing destroyed mutex behavior\n");

  mid = mutex_create();
  
  check(mid >= 0, "create_mutex() failed");
  check(!mutex_destroy(mid), "destroy_mutex() failed");
  check(mutex_destroy(mid) == -1, "destroy succeeded after already destroyed");
  check(mutex_lock(mid) == -1, "lock succeeded after destroy");
  check(mutex_unlock(mid) == -1, "unlock succeeded after destroy");
}

void
test_high_load(void)
{
  int nmutex = 12;
  int ppm = 2; // procs per mutex
  int nuses = 4; // locks/unlocks per proc

  printf(1, "load testing\n");

  while (nmutex--) {
    if (fork() < 0) {
      int m = mutex_create();
      int i;
      
      check(m >= 0, "mutex_create() failed");

      for (i = 0; i < ppm; i++) {
        if (fork() == 0) {
          for (i = 0; i < nuses; i++) {
            check(!mutex_lock(m), "lock failed");
            check(!mutex_unlock(m), "unlock failed");            
          }
          exit();
        }
      }
      while (wait() > 0)
        ;
      check(!mutex_destroy(m), "destroy failed");
      exit();
    }
  }
  while (wait() > 0)
    ;
}

int
main(void)
{
  test_syscalls();
  test_mutual_exclusion();
  test_mutual_exclusion2();
  test_create_multiple();
  test_bad_args();
  test_destroy();
  test_high_load();
  exit();
}
