#ifndef __MUTEXLOCK_H__
#define __MUTEXLOCK_H__

#include "types.h"
#include "spinlock.h"

struct mutexlock{
    struct spinlock lk;
    uint locked;    //lock held?
    uint ref;  //to keep track of references
};

#define NUM_MUTEX   32

struct {
    struct spinlock lk;
    struct mutexlock mutex[NUM_MUTEX];
}mtable;

#endif
