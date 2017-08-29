#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

int one_meg_test ()
{
  int fds[2];
  pid_t pid;
  int BUFFER_SIZE = 1024*1024;
  int bytes_read = 0;

  if (pipe(fds) != 0) 
  { 
     perror("pipe"); 
     exit(1); 
  } 

  char* write_buffer = (char*) malloc (BUFFER_SIZE);
  char* read_buffer = (char*) malloc (BUFFER_SIZE);
  
  int i;
  for(i = 0; i < BUFFER_SIZE; ++i)
  {
    write_buffer[i] = (unsigned char) (rand() % 255);
  }

  if ((pid = fork()) == -1)
  {
    perror("fork");
    exit(1);
  }
 
  if (pid == 0)
  {
    close(fds[0]);
    int bytes_written = 0;
    bytes_written = write(fds[1], write_buffer, BUFFER_SIZE);
    assert(bytes_written == BUFFER_SIZE);
    exit(0);
  }
  else
  {
    close(fds[1]);
    bytes_read = read(fds[0], read_buffer, BUFFER_SIZE);
    printf("Read %d bytes\n", bytes_read);

    // Make sure bytes read == bytes written. 
    printf("Expecting %d bytes\n", BUFFER_SIZE);
    assert(BUFFER_SIZE == bytes_read);
    printf("%d bytes read\n   PASS\n", bytes_read);

    // Make sure buffers match.
    printf("Expecting buffer contents to match\n");
    for (i = 0; i < BUFFER_SIZE; ++i)
    {
      assert(read_buffer[i] == write_buffer[i]);
    }
    printf("Buffer contents match\n  PASS\n");

    return 0;
  }
}

int main ()
{
  if (one_meg_test() == 0)
  {
    printf("one_meg_test - PASSED\n");
  }
  else
  {
    printf("one_meg_test - FAILED\n");
  }
}
