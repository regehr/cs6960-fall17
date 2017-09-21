#include "mutexlock.h"
#include "defs.h"

void mtableinit(void) {
    initlock(&mtable.lk, "mtable");

    for (struct mutexlock* m = mtable.mutex; m < mtable.mutex + NUM_MUTEX; m++) {
        initlock(&m->lk, "mutexlock");
        m->ref = 0;
        m->locked = 0;
    }
}

struct mutexlock* getmutex(int mid) {
    if (mid < 0 || mid >= NUM_MUTEX)
        return 0;
    
    if (mtable.mutex[mid].ref == 0)
        return 0;
  
    return mtable.mutex + mid;
}

int sys_create_mutex(void) {
    int* mid;
    
    if (argptr(0, (void*)&mid, sizeof(mid[0])) < 0)
        return -1;
    
    acquire(&mtable.lk);

    for(int i = 0; i < NUM_MUTEX; i++) {
        if(mtable.mutex[i].ref == 0) {
            
            mtable.mutex[i].ref = 1;

            *mid = i;
            
            release(&mtable.lk);
            return 0;
        }
    }

    release(&mtable.lk);

    return -1;
}

int sys_acquire_mutex(void) {
    int mid;
    
    if (argint(0, &mid) < 0)
        return -1;
   
    acquire(&mtable.lk);

    struct mutexlock *m = getmutex(mid);
    if (!m) {
        release(&mtable.lk);
        return -1;
    }
    
    release(&mtable.lk);

    acquire(&m->lk);
    
    while (m->locked) {
        sleep(m, &m->lk);    
    }
    m->locked = 1;
    
    release(&m->lk);

    return 0;
}

int sys_release_mutex(void) {
    int mid;
    if (argint(0, &mid) < 0)
        return -1;

    acquire(&mtable.lk);

    struct mutexlock *m = getmutex(mid);
    if (!m) {
        release(&mtable.lk);
        return -1;
    }
    
    acquire(&m->lk);
    
    m->locked = 0;
    
    wakeup(m);

    release(&m->lk);
    release(&mtable.lk);

    return 0;
}

int sys_delete_mutex(void) {
    int mid;
    if (argint(0, &mid) < 0)
        return -1;

    acquire(&mtable.lk);

    struct mutexlock *m = getmutex(mid);
    if (!m) {
        release(&mtable.lk);
        return -1;
    }
    
    acquire(&m->lk);

    if(m->ref == 0) {
        release(&m->lk);
        release(&mtable.lk);
        return -1;
    }

    m->ref = 0;

    release(&m->lk);
    release(&mtable.lk);

    return 0;
}
