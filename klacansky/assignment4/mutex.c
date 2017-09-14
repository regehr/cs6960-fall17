#include "types.h"
#include "user.h"
#include "mutex.h"

int
mutexcreate(struct mutex *const m)
{
  if (pipe(m->pipe) == -1)
    return -1;

  // TODO: assumes pipe can hold at least one byte
  char tmp = 0;
  if (write(m->pipe[1], &tmp, 1) == -1)
    return -1;

  return 0;
}

int
mutexacquire(struct mutex *const m)
{
  char tmp;
  return read(m->pipe[0], &tmp, 1);
}

int
mutexrelease(struct mutex *const m)
{
  char tmp = 0;
  return write(m->pipe[1], &tmp, 1);
}

int
mutexdestroy(struct mutex *const m)
{
  if (close(m->pipe[0]) == -1)
    return -1;
  return close(m->pipe[1]);
}
