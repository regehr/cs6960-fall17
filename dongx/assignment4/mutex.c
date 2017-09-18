#include "types.h"
#include "defs.h"
#include "spinlock.h"

#define NMUTEX 64

struct mutex {
    int hold;
    struct spinlock lk;
};

enum mutex_state {
    USED, UNUSED
};

struct {
    struct spinlock lk;
    enum mutex_state states[NMUTEX];
    struct mutex mus[NMUTEX];
} mutable;

void
mutable_init(void)
{
    initlock(&mutable.lk, "mutable");

    acquire(&mutable.lk);
    for (int i = 0; i < NMUTEX; ++i) {
        mutable.states[i] = UNUSED;
        initlock(&(mutable.mus[i].lk), "mutex");
        mutable.mus[i].hold = 0;
    }

    release(&mutable.lk);
}

int
mutex_create(void)
{
    acquire(&mutable.lk);

    for (int i = 0; i < NMUTEX; ++i) {
        if (mutable.states[i] == UNUSED) {
            mutable.states[i] = USED;
            release(&mutable.lk);
            return i;
        }
    }

    release(&mutable.lk);
    return -1;
}

int
mutex_acquire(int mu_id)
{
    if (mu_id < 0 || mu_id >= NMUTEX || mutable.states[mu_id] == UNUSED)
        return -1;

    struct mutex* mu = &(mutable.mus[mu_id]);
    acquire(&mu->lk);
    while (mu->hold) {
        sleep(mu, &mu->lk);
    }
    mu->hold = 1;
    release(&mu->lk);
    return 0;
}

int
mutex_release(int mu_id) {
    if (mu_id < 0 || mu_id >= NMUTEX || mutable.states[mu_id] == UNUSED)
       return -1;

    struct mutex* mu = &(mutable.mus[mu_id]);
    acquire(&mu->lk);
    mu->hold = 0;
    wakeup(mu);
    release(&mu->lk);
    return 0;
}

int
mutex_destroy(int mu_id) {
    acquire(&mutable.lk);
    if (mu_id < 0 || mu_id >= NMUTEX || mutable.states[mu_id] == UNUSED) {
        release(&mutable.lk);
        return -1;
    }
    mutable.states[mu_id] = UNUSED;    
    release(&mutable.lk);
    return 0;
}