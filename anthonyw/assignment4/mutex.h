#define NMTX 64

#define TAG_UNALLOC 0

struct mutex {
  int locked;
  int tag;
  int pid;
  struct spinlock lock;
};

int mutex_create(int tag);
int mutex_acquire(int tag);
int mutex_release(int tag);
int mutex_destroy(int tag);
