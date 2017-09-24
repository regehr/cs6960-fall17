#include "types.h"
#include "defs.h"
#include "spinlock.h"

#define NULL 0
#define TRUE 1
#define FALSE 0

#define MUTEX_COUNT 8

struct mutex {
    int acquired;
    struct spinlock lock;
};

struct {
    struct mutex list[MUTEX_COUNT];
    struct spinlock lock;
} mutexes;

int
id_is_valid(int mutex_id) {
    if (mutex_id < 0 || mutex_id >= MUTEX_COUNT) {
        return FALSE;
    }
    return TRUE;
}

struct mutex*
get_mutex(int mutex_id) {
    if (id_is_valid(mutex_id) == FALSE) {
        return NULL;
    }

    return &mutexes.list[mutex_id];
}

void
initialize_mutexes(void) {
    initlock(&mutexes.lock, "mutexes");

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        struct mutex* mtx = get_mutex(i);
        initlock(&mtx->lock, "mutex");
        mtx->acquired = FALSE;
    }
}

int
mutex_create(void) {
    // Acquire lock on table and iterate through mutexes.
    acquire(&mutexes.lock);
    {
        for (int i = 0; i < MUTEX_COUNT; ++i) {
            // Check if mutex is free.
            if (mutexes.list[i].acquired == FALSE) {
                // Acquire mutex; return success.
                mutexes.list[i].acquired = TRUE;
                release(&mutexes.lock);
                return i;
            }
        }
    }
    release(&mutexes.lock);

    return -1;
}

int
mutex_acquire(int mutex_id) {
    struct mutex* mtx = get_mutex(mutex_id);
    if (mtx == NULL) {
        return -1;
    }

    acquire(&mtx->lock);
    {
        while (mtx->acquired) {
            sleep(mtx, &mtx->lock);
        }
        mtx->acquired = TRUE;
    }
    release(&mtx->lock);

    return 0;
}

int
mutex_release(int mutex_id) {
    struct mutex* mtx = get_mutex(mutex_id);
    if (mtx == NULL) {
        return -1;
    }

    acquire(&mtx->lock);
    {
        if (mtx->acquired == FALSE) {
            release(&mtx->lock);
            return -1;
        }

        mtx->acquired = TRUE;
        wakeup(mtx);
    }
    release(&mtx->lock);

    return 0;
}

int
mutex_destroy(int mutex_id) {
    if (!id_is_valid(mutex_id)) {
        return -1;
    }

    acquire(&mutexes.lock);
    {
        if (mutexes.list[mutex_id].acquired == FALSE) {
            release(&mutexes.lock);
            return -1;
        }

        mutexes.list[mutex_id].acquired = FALSE;
    }
    release(&mutexes.lock);

    return 0;
}
