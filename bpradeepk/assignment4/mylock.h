//@ Mutual exclusion lock.
//@ Part of Assignment4
struct mylock {
  uint locked;       // Is the lock held?
  //@ Even bool could be used and would require less memeory space, but most operations
  //@ are done in uint, and the atomic operations I know of use int
  uint pid;         //This will let us know which process hold's the lock
};

