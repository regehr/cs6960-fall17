#include "types.h"
#include "defs.h"

int
sys_mutex_create(void)
{
    return mutex_create();
}

int
sys_mutex_acquire(void)
{
    int mu_id;
    if (argint(0, &mu_id) < 0)
        return -1;
    
    return mutex_acquire(mu_id);
}

int
sys_mutex_release(void)
{
    int mu_id;
    if (argint(0, &mu_id) < 0)
        return -1;
    
    return mutex_release(mu_id);
}

int
sys_mutex_destroy(void)
{
    int mu_id;
    if (argint(0, &mu_id) < 0)
        return -1;
    
    return mutex_destroy(mu_id);
}