#include "types.h"
#include "user.h"

void assert(int x, int n) 
{
  if(!x)
  {
    printf(1, "assertion %d failed, exiting\n", n);
    exit();
  }
}

void one_meg_test ()
{
  int fds[2];
  int pid;
  int BUFFER_SIZE = 1024*1024;
  int bytes_read = 0;

  if (pipe(fds) != 0) 
  { 
     printf(1, "Error opening pipe\n"); 
     exit(); 
  } 

  char* write_buffer = (char*) malloc (BUFFER_SIZE);
  char* read_buffer = (char*) malloc (BUFFER_SIZE);

  int i;
  for(i = 0; i < BUFFER_SIZE; ++i)
  {
    unsigned int s = 1;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    write_buffer[i] = s;
  }

  if ((pid = fork()) == -1)
  {
    printf(1, "Error forking\n");
    exit();
  }
 
  if (pid == 0)
  {
    close(fds[0]);
    int bytes_written = 0;
    bytes_written += write(fds[1], write_buffer, BUFFER_SIZE);
    printf(1, "%d bytes written to pipe\n", bytes_written);
    exit();
  }
  else
  {
    close(fds[1]);
    char buffer[512];
    int buff_read = 0;
    while (bytes_read < BUFFER_SIZE)
    {
      buff_read = read(fds[0], buffer, 512);
      for (int i = 0; i < buff_read; ++i)
      {
        read_buffer[bytes_read++] = buffer[i];
      }
    }
    printf(1, "Read %d bytes\n", bytes_read);

    // Make sure bytes read == bytes written. 
    printf(1, "Expecting %d bytes\n", BUFFER_SIZE);
    if (bytes_read != BUFFER_SIZE)
    {
      printf(1, "Bytes read %d\n", bytes_read);
    }

    // Make sure buffers match.
    printf(1, "Expecting buffer contents to match\n");
    for (i = 0; i < BUFFER_SIZE; ++i)
    {
      if (read_buffer[i] !=  write_buffer[i])
      {
        printf(1, "Buffers do not match on %dth element, FAIL\n", i);
      }
    }
    wait();
  }
  printf(1, "Passed 1MB test\n");
}

int main ()
{
  one_meg_test();

  return 0;
}
