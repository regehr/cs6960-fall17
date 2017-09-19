#include "types.h"
#include "defs.h"

int sys_mtx_acquire(void)
{
  int mu_id;
  if (argint(0, &mu_id) < 0)
    return -1;
    
  return mtx_acquire(mu_id);
}

int sys_mtx_release(void)
{
  int mu_id;
  if (argint(0, &mu_id) < 0)
    return -1;
    
  return mtx_release(mu_id);
}
 

int sys_mtx_create(void)
{
  return mtx_create();
}


int sys_mtx_destroy(void)
{
  int mu_id;
  if (argint(0, &mu_id) < 0)
    return -1;
    
  return mtx_destroy(mu_id);
}
