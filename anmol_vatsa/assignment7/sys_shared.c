/**
 * Some code borrowed from alex_steele and dongx
 */
#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"

int sys_shared_mem() {
  return map_shared(myproc()->pgdir);
}
