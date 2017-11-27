
#include "types.h"
#include "defs.h"

int
sys_mutex_create(void)
{
  return mutexcreate();
}

int sys_mutex_lock(void)
{
  int mid;
  if (argint(0, &mid) < 0)
    return -1;
  return mutexlock(mid);
}

int
sys_mutex_unlock(void)
{
  int mid;
  if (argint(0, &mid) < 0)
    return -1;
  return mutexunlock(mid);
}

int
sys_mutex_destroy(void)
{
  int mid;
  if (argint(0, &mid) < 0)
    return -1;
  return mutexdestroy(mid);
}
